#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/of_device.h>
#include <linux/ioport.h> 
#include <linux/sizes.h>
#include "platform.h"

/* global memory variable */
void __iomem *gpmc_base;
unsigned long fpga_base = 0;
unsigned short  *gpmc_cs0_pointer = 0 ;
unsigned short *gpmc_cs2_pointer=0;
static struct resource	gpmc_mem_root ;
static struct gpmc_cs_data gpmc_cs[7];
static DEFINE_SPINLOCK(gpmc_mem_lock) ;
unsigned int * gpmc_reg_pointer;
volatile unsigned int *ctrl_reg_pointer;



/* device data structure */
struct gpmcdev_private_data
{
    struct gpmcdev_platform_data pdata;
    char* buffer;
    dev_t dev_num;
    struct cdev cdev;
};

/* driver data structure */
struct gpmcdrv_private_data
{
    int total_devices;
    dev_t device_num_base;
    struct class *class_gpmc;
    struct device *device_gpmc;
    struct gpmcdev_private_data gpmcdev_data;
};
struct gpmcdrv_private_data gpmcdrv_data;

enum pcdev_names 
{
    PCDEVA1X,
    PCDEVB1X,
    PCDEVC1X,
    PCDEVD1X
};
/* configuration item to config device */
struct device_config
{
    int config_item1;
    int config_item2;
};

struct device_config pcdev_config[] = 
{
    [PCDEVA1X] = {.config_item1 = 60, .config_item2 = 21}, 
    [PCDEVB1X] = {.config_item1 = 50, .config_item2 = 22}, 
    [PCDEVC1X] = {.config_item1 = 40, .config_item2 = 23}, 
    [PCDEVD1X] = {.config_item1 = 30, .config_item2 = 24} 
}; 

void init_gpmc_bases(void) 
{
    gpmc_base = ioremap_nocache(gpmcdrv_data.gpmcdev_data.pdata.gpmc_base[0], gpmcdrv_data.gpmcdev_data.pdata.gpmc_base[1]); 
    request_mem_region(fpga_base, gpmcdrv_data.gpmcdev_data.pdata.block_size * 2 * sizeof(unsigned long), "fpga");
	gpmc_cs0_pointer = ioremap_nocache(fpga_base, gpmcdrv_data.gpmcdev_data.pdata.block_size * 2 * sizeof(unsigned long));
    gpmc_cs2_pointer = ioremap_nocache(fpga_base, gpmcdrv_data.gpmcdev_data.pdata.block_size * 2 * sizeof(unsigned long));
    release_mem_region(fpga_base, gpmcdrv_data.gpmcdev_data.pdata.block_size * 2 * sizeof(unsigned long));
}

void uninit_gpmc_bases(void)
{
    iounmap(gpmc_reg_pointer);
    iounmap(ctrl_reg_pointer);
    iounmap(gpmc_base);
    iounmap(gpmc_cs0_pointer);
    iounmap(gpmc_cs2_pointer);
}

/**
* @brief  gpmc_cs_write_reg 
*         write value to GPMC register in AM335x cpu
* @param  cs    : chip select number 
*         idx   : register address from cs offset 
*         value : value written to register
* @retval None
*/
void gpmc_cs_write_reg(int cs, int idx, u32 val)
{
	void __iomem *reg_addr;
	reg_addr = gpmc_base + GPMC_CS0_OFFSET + (cs * GPMC_CS_SIZE) + idx;
	//GPMC_CS_SIZE : size of dedicated GPMC chip select
	//
	writel_relaxed(val, reg_addr);
}

/**
* @brief  gpmc_cs_read_reg 
*         read value from GPMC register in AM335x cpu
* @param  cs    : chip select number 
*         idx   : register address from cs offset 
* @retval reg value
*/
static u32 gpmc_cs_read_reg(int cs, int idx)
{
	void __iomem *reg_addr;
	reg_addr = gpmc_base + GPMC_CS0_OFFSET + (cs * GPMC_CS_SIZE) + idx;
	return readl_relaxed(reg_addr);
}

/**
* @brief  gpmc_cs_set_reserved 
*         set flag for selected GPMC cs
* @param  cs        : chip select number 
*         reserved  : flag 
* @retval none
*/
static void gpmc_cs_set_reserved(int cs, int reserved)
{
	struct gpmc_cs_data *gpmc = &gpmc_cs[cs];

	gpmc->flags |= GPMC_CS_RESERVED;
}

/**
* @brief  gpmc_cs_mem_enabled 
*         check GPMC cs valid
* @param  cs        : chip select number 
* @retval status
*/
static int gpmc_cs_mem_enabled(int cs){
	u32 l;
	l = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG7);
	printk("cs_mem_enable\r\n") ;
	return l & GPMC_CONFIG7_CSVALID;
}

/**
* @brief  gpmc_cs_enable_mem 
*         enable selected GPMC cs region
* @param  cs   : chip select number 
* @retval none
*/
static void gpmc_cs_enable_mem(int cs)
{
	u32 l;

	l = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG7);
	l |= GPMC_CONFIG7_CSVALID;
	gpmc_cs_write_reg(cs, GPMC_CS_CONFIG7, l);
}

/**
* @brief  gpmc_cs_disable_mem 
*         disable selected GPMC cs region
* @param  cs        : chip select number 
* @retval none
*/
static void gpmc_cs_disable_mem(int cs){
	u32 l;
	l = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG7);
	l &= ~GPMC_CONFIG7_CSVALID;
	gpmc_cs_write_reg(cs, GPMC_CS_CONFIG7, l);
}

static unsigned long gpmc_mem_align(unsigned long size)
{
	int order;

	size = (size - 1) >> (GPMC_CHUNK_SHIFT - 1);
	order = GPMC_CHUNK_SHIFT - 1;
	do {
		size >>= 1;
		order++;
	} while (size);
	size = 1 << order;
	return size;
}

static int gpmc_cs_insert_mem(int cs, unsigned long base, unsigned long size)
{
	struct gpmc_cs_data *gpmc = &gpmc_cs[cs];
	struct resource *res = &gpmc->mem;
	int r;
	size = gpmc_mem_align(size);
	spin_lock(&gpmc_mem_lock);
	res->start = base;
	res->end = base + size - 1;
	//printk(KERN_ALERT "GPMC size align is %d base is %d\n", size, base);
	r = request_resource(&gpmc_mem_root, res);
	spin_unlock(&gpmc_mem_lock);
	return r;
}
/**
* @brief  gpmc_cs_set_memconf 
*         configure GPMC based address and size 
* @param  cs   : chip select number
*         base : base address
*         size : memory size  
* @retval 0
*/
static int gpmc_cs_set_memconf(int cs, u32 base, u32 size){
	u32 l;
	u32 mask;
	/*
	 * Ensure that base address is aligned on a
	 * boundary equal to or greater than size.
	 */
	if (base & (size - 1))
		return -EINVAL;
	base >>= GPMC_CHUNK_SHIFT;
	mask = (1 << GPMC_SECTION_SHIFT) - size;
	mask >>= GPMC_CHUNK_SHIFT;
	mask <<= GPMC_CONFIG7_MASKADDRESS_OFFSET;
	l = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG7);
	l &= ~GPMC_CONFIG7_MASK;
	l |= base & GPMC_CONFIG7_BASEADDRESS_MASK;
	l |= mask & GPMC_CONFIG7_MASKADDRESS_MASK;
	l |= GPMC_CONFIG7_CSVALID;
	printk("gpmc_cs_set_memconf cs%d %x\r\n", cs, l);
	gpmc_cs_write_reg(cs, GPMC_CS_CONFIG7, l);
	return 0;
}

/**
* @brief  gpmc_cs_get_memconf 
*         get configured memory size of selected cs
* @param  cs    : chip select number 
*         *base : base address 
*         *size : memory size  
* @retval none
*/
static void gpmc_cs_get_memconf(int cs, u32 *base, u32 *size)
{
	u32 l;
	u32 mask;
	l = gpmc_cs_read_reg(cs, GPMC_CS_CONFIG7);
	*base = (l & 0x3f) << GPMC_CHUNK_SHIFT;
	mask = (l >> 8) & 0x0f;
	*size = (1 << GPMC_SECTION_SHIFT) - (mask << GPMC_CHUNK_SHIFT);
	printk(KERN_ALERT "GPMC cs%d base is %d size is %d\n", cs , *base, *size);
}

/**
* @brief  gpmc_mem_init 
*         initialize GPMC
* @param  none 
* @retval none
*/
static void gpmc_mem_init(void){
	int cs;
	/*
	 * The first 1MB of GPMC address space is 
	 mapped to
	 * the internal ROM. Never allocate the first page, to
	 * facilitate bug detection; even if we didn't boot from ROM.
	 */
	 printk("mem_init 1.1\n");
	gpmc_mem_root.start = SZ_1M;
	gpmc_mem_root.end = GPMC_MEM_END;
	printk("mem_init 1\n");
	/* Reserve all regions that has been set up by bootloader */
	for (cs = 0; cs < gpmcdrv_data.gpmcdev_data.pdata.gpmc_cs_num; cs++) {
		u32 base, size;
		printk("mem_init 2\n");
		if (!gpmc_cs_mem_enabled(cs))
			continue;
		printk("mem_init 4\n");
		gpmc_cs_get_memconf(cs, &base, &size);
		printk("mem_init 5\n");
		if (gpmc_cs_insert_mem(cs, base, size)) {
			pr_warn("%s: disabling cs %d mapped at 0x%x-0x%x\n",
				__func__, cs, base, base + size);
			gpmc_cs_disable_mem(cs);
		}
		printk("mem_init 6\n");
	}
	printk("mem_init 3\n");
}

static int gpmc_cs_delete_mem(int cs)
{
	struct gpmc_cs_data *gpmc = &gpmc_cs[cs];
	struct resource *res = &gpmc->mem;
	int r;
	spin_lock(&gpmc_mem_lock);
	r = release_resource(res);
	res->start = 0;
	res->end = 0;
	spin_unlock(&gpmc_mem_lock);
	return r;
}

static bool gpmc_cs_reserved(int cs)
{
	struct gpmc_cs_data *gpmc = &gpmc_cs[cs];
	return gpmc->flags & GPMC_CS_RESERVED;
}

static void gpmc_mem_exit(void)
{
	int cs;
	for (cs = 0; cs < gpmcdrv_data.gpmcdev_data.pdata.gpmc_cs_num; cs++) {
		if (!gpmc_cs_mem_enabled(cs))
			continue;
		gpmc_cs_delete_mem(cs);
	}
}

//=====================================
//calculate selected cs start address 
//then assign and enable 
//=====================================
int gpmc_cs_request(int cs, unsigned long size, unsigned long *base){
	struct gpmc_cs_data *gpmc = &gpmc_cs[cs];
	struct resource *res = &gpmc->mem;
	int r = -1;
	if (cs > gpmcdrv_data.gpmcdev_data.pdata.gpmc_cs_num) {
		pr_err("%s: requested chip-select is disabled\n", __func__);
		return -ENODEV;
	}
	size = gpmc_mem_align(size);
	printk("gpmc_cs_request 1\n");
	if (size > (1 << GPMC_SECTION_SHIFT))
		return -ENOMEM;
	printk("gpmc_cs_request 2\n");
	spin_lock(&gpmc_mem_lock);
	if (gpmc_cs_reserved(cs)) {
		r = -EBUSY;
		goto out;
	}
	printk("gpmc_cs_request 3\n");
	if (gpmc_cs_mem_enabled(cs))
		r = adjust_resource(res, res->start & ~(size - 1), size);
	if (r < 0)
		r = allocate_resource(&gpmc_mem_root, res, size, 0, ~0,
				      size, NULL, NULL);
	printk("gpmc_cs_request memory allocation %x\r\n", (int)res->start);
	printk("gpmc_cs_request 4\n");
	if (r < 0)
		goto out;
	/* Disable CS while changing base address and size mask */
	gpmc_cs_disable_mem(cs);
	printk("gpmc_cs_request 5\n");
	r = gpmc_cs_set_memconf(cs, res->start, resource_size(res));
	if (r < 0) {
		release_resource(res);
		goto out;
	}
	/* Enable CS */
	gpmc_cs_enable_mem(cs);
	printk("gpmc_cs_request 6\n");
	*base = res->start;
	gpmc_cs_set_reserved(cs, 1);
out:
	spin_unlock(&gpmc_mem_lock);
	return r;
}

void orShortRegister(unsigned short int value, volatile unsigned int * port)
{
	unsigned short oldVal ;
	oldVal = ioread32((void __iomem *)port);
	iowrite32(oldVal | value, (void __iomem *)port);
}

//===========================
//Setup BBB i/o mode to GPMC
//===========================
int mode_setup(void){
    int soc_ctrl_regs = gpmcdrv_data.gpmcdev_data.pdata.ctrl_base;
    int i;
	request_mem_region(soc_ctrl_regs, 720, "gDrvrName");
	ctrl_reg_pointer = ioremap_nocache(soc_ctrl_regs,  720);
	
	//printk(KERN_ALERT "GPMC_AD0 value : %X \n", ioread32(ctrl_reg_pointer + GPMC_AD0/4));
    iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 0, CTRL_AD_OFFSET))); //AD0
	
	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 1, CTRL_AD_OFFSET))); //AD1

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 2, CTRL_AD_OFFSET))); //AD2
	
	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 3, CTRL_AD_OFFSET))); //AD3
	
	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 4, CTRL_AD_OFFSET))); //AD4

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 5, CTRL_AD_OFFSET))); //AD5

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 6, CTRL_AD_OFFSET))); //AD6

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 7, CTRL_AD_OFFSET))); //AD7

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 8, CTRL_AD_OFFSET))); //AD8

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 9, CTRL_AD_OFFSET))); //AD9

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 10, CTRL_AD_OFFSET))); //AD10

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 11, CTRL_AD_OFFSET))); //AD11

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 12, CTRL_AD_OFFSET))); //AD12

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 13, CTRL_AD_OFFSET))); //AD13

	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 14, CTRL_AD_OFFSET))); //AD14
	
	iowrite32(MODE0 | RX_ACTIVE, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0, 15, CTRL_AD_OFFSET))); //AD15

	for(i = 0; i < gpmcdrv_data.gpmcdev_data.pdata.gpmc_cs_num; i++) {
        if (gpmcdrv_data.gpmcdev_data.pdata.gpmc_cs[i] == 1)
            iowrite32(MODE0, (void __iomem *)(ctrl_reg_pointer + GPMC_OFFSET(CTRL_CS0, i, CTRL_CS_OFFSET)));
    }
	iowrite32(MODE0, (void __iomem *)(ctrl_reg_pointer + CTRL_WEN/4));
	
	iowrite32(MODE0, (void __iomem *)(ctrl_reg_pointer + CTRL_ADVN/4));
	
	iowrite32(MODE0, (void __iomem *)(ctrl_reg_pointer + CTRL_OEN/4));
	
	/*iowrite32(MODE0, ctrl_reg_pointer + GPMC_A16/4);
	
	iowrite32(MODE0, ctrl_reg_pointer + GPMC_A17/4);
	
	iowrite32(MODE0, ctrl_reg_pointer + GPMC_A18/4);
	
	iowrite32(MODE0 | RX_ACTIVE, ctrl_reg_pointer + GPMC_CLK/4);
	
	iowrite32(MODE0, ctrl_reg_pointer + GPMC_BE0/4);
	
	iowrite32(MODE0, ctrl_reg_pointer + GPMC_BE1/4);*/

	release_mem_region(soc_ctrl_regs, 720);
	return 0 ; 
}

//=================================
//Configure GPMC timming parameter
//=================================
int setupGPMCNonMuxed(void){
	unsigned int temp = 0;	
    int i;	
	printk("Configuring GPMC for  muxed access \n");	
	request_mem_region(gpmcdrv_data.gpmcdev_data.pdata.gpmc_base[0], 720, "gDrvrName");
	gpmc_reg_pointer = ioremap_nocache(gpmcdrv_data.gpmcdev_data.pdata.gpmc_base[0], 720);
	printk("GPMC_REVISION value :%x \n", ioread32(gpmc_reg_pointer + GPMC_REVISION/4)); 
	
	orShortRegister(GPMC_SYSCONFIG_SOFTRESET, gpmc_reg_pointer + GPMC_SYSCONFIG/4 ) ;
	printk("Trying to reset GPMC \n"); 
	printk("GPMC_SYSSTATUS value :%x \n", ioread32(gpmc_reg_pointer + GPMC_SYSSTATUS/4)); 
	while((ioread32(gpmc_reg_pointer + GPMC_SYSSTATUS/4) & 
		GPMC_SYSSTATUS_RESETDONE) == GPMC_SYSSTATUS_RESETDONE_RSTONGOING){
		printk("GPMC_SYSSTATUS value :%x \n", ioread32(gpmc_reg_pointer + 
		GPMC_SYSSTATUS/4));
	}
	printk("GPMC reset \n");
	temp = ioread32(gpmc_reg_pointer + GPMC_SYSCONFIG/4);
	temp &= ~GPMC_SYSCONFIG_IDLEMODE;
	temp |= GPMC_SYSCONFIG_IDLEMODE_NOIDLE << GPMC_SYSCONFIG_IDLEMODE_SHIFT;
	iowrite32(temp, gpmc_reg_pointer + GPMC_SYSCONFIG/4);
	iowrite32(0x00, gpmc_reg_pointer + GPMC_IRQENABLE/4) ;
	iowrite32(0x00, gpmc_reg_pointer + GPMC_TIMEOUT_CONTROL/4);
    
    for(i = 0; i < gpmcdrv_data.gpmcdev_data.pdata.gpmc_cs_num; i++) {
        if(gpmcdrv_data.gpmcdev_data.pdata.gpmc_cs[i] != 0) {
            iowrite32((0x0 |
            (GPMC_CONFIG1_0_DEVICESIZE_SIXTEENBITS << GPMC_CONFIG1_0_DEVICESIZE_SHIFT ) |
            (GPMC_CONFIG1_0_MUXADDDATA_MUX << GPMC_CONFIG1_0_MUXADDDATA_SHIFT ) | 
            (GPMC_CONFIG1_0_GPMCFCLKDIVIDER_DIVBY1 << GPMC_CONFIG1_0_GPMCFCLKDIVIDER_SHIFT) |
            (GPMC_CONFIG1_0_CLKACTIVATIONTIME_ONECLKB4 << GPMC_CONFIG1_0_CLKACTIVATIONTIME_SHIFT)), 
            gpmc_reg_pointer + GPMC_CONFIG1(i)) ;	//Address/Data multiplexed
            printk("GPMC_CONFIG1 value :%x \n", ioread32(gpmc_reg_pointer + GPMC_CONFIG1(i))); 
            
            iowrite32((0x0 |
            (CS_ON) |	// CS_ON_TIME
            (CS_OFF << GPMC_CONFIG2_0_CSRDOFFTIME_SHIFT) |	// CS_DEASSERT_RD
            (CS_OFF << GPMC_CONFIG2_0_CSWROFFTIME_SHIFT)),	//CS_DEASSERT_WR
            gpmc_reg_pointer + GPMC_CONFIG2(i))  ;	
            printk("GPMC_CONFIG2 value :%x \n", ioread32(gpmc_reg_pointer + GPMC_CONFIG2(i))); 

            iowrite32((0x0 |
            (ADV_ON << GPMC_CONFIG3_0_ADVONTIME_SHIFT) | //ADV_ASSERT
            (ADV_OFF << GPMC_CONFIG3_0_ADVRDOFFTIME_SHIFT) | //ADV_DEASSERT_RD
            (ADV_OFF << GPMC_CONFIG3_0_ADVWROFFTIME_SHIFT)), //ADV_DEASSERT_WR
            gpmc_reg_pointer + GPMC_CONFIG3(i)) ; 
            printk("GPMC_CONFIG3 value :%x \n", ioread32(gpmc_reg_pointer + GPMC_CONFIG3(i)));

            iowrite32( (0x0 |
            (OE_ON << GPMC_CONFIG4_0_OEONTIME_SHIFT) |	//OE_ASSERT
            (OE_OFF << GPMC_CONFIG4_0_OEOFFTIME_SHIFT) |	//OE_DEASSERT
            (WR_ON << GPMC_CONFIG4_0_WEONTIME_SHIFT)| //WE_ASSERT
            (WR_OFF << GPMC_CONFIG4_0_WEOFFTIME_SHIFT)),  //WE_DEASSERT
            gpmc_reg_pointer + GPMC_CONFIG4(i))  ; 
            printk("GPMC_CONFIG4 value :%x \n", ioread32(gpmc_reg_pointer + GPMC_CONFIG4(i)));	

            iowrite32((0x0 |
            (RD_CYC << GPMC_CONFIG5_0_RDCYCLETIME_SHIFT)|	//CFG_5_RD_CYCLE_TIM
            (WR_CYC << GPMC_CONFIG5_0_WRCYCLETIME_SHIFT)|	//CFG_5_WR_CYCLE_TIM
            (RD_ACC_TIME << GPMC_CONFIG5_0_RDACCESSTIME_SHIFT)),	// CFG_5_RD_ACCESS_TIM
            gpmc_reg_pointer + GPMC_CONFIG5(i))  ;  
            printk("GPMC_CONFIG5 value :%x \n", ioread32(gpmc_reg_pointer + GPMC_CONFIG5(i))); 
            
            iowrite32((0x0 |
            (0 << GPMC_CONFIG6_0_CYCLE2CYCLESAMECSEN_SHIFT) |
            (0 << GPMC_CONFIG6_0_CYCLE2CYCLEDELAY_SHIFT) | //CYC2CYC_DELAY
            (WRDATAONADMUX << GPMC_CONFIG6_0_WRDATAONADMUXBUS_SHIFT)| //WR_DATA_ON_ADMUX
            (0 << GPMC_CONFIG6_0_WRACCESSTIME_SHIFT)), //CFG_6_WR_ACCESS_TIM
            gpmc_reg_pointer + GPMC_CONFIG6(i)) ; 
            printk("GPMC_CONFIG6 value :%x \n", ioread32(gpmc_reg_pointer + GPMC_CONFIG6(i))); 	

            iowrite32( //CFG_7_BASE_ADDR default = 0x1000 
            (0x01 << GPMC_CONFIG7_0_CSVALID_SHIFT ) | 
            (GPMC_CONFIG7_0_BASEADDRESS << GPMC_CONFIG7_0_BASEADDRESS_SHIFT) | //CFG7_BASE
            (GPMC_CONFIG7_0_MASKADDRESS << GPMC_CONFIG7_0_MASKADDRESS_SHIFT) ,  //CFG_7_MASK	
            gpmc_reg_pointer + GPMC_CONFIG7(i));
            printk("GPMC_CONFIG7 value :%x \n", ioread32(gpmc_reg_pointer + GPMC_CONFIG7(i))); 
        }
    }
	release_mem_region(gpmcdrv_data.gpmcdev_data.pdata.gpmc_base[0], 720);
	return 1;
}

int check_permission(int dev_perm, int acc_mode)
{
    if(dev_perm == RDWR)
        return 0;
    if(dev_perm == RDONLY && ( (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)) )
        return 0;
    if(dev_perm == WRONLY && ( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)) )
        return 0;
    return -EPERM;
}

int gpmc_open(struct inode *inode, struct file *filp)
{
    return 0;
}

int gpmc_release(struct inode *inode, struct file *filp)
{
    pr_info("release requsted\r\n");
    return 0;
}

ssize_t gpmc_read(struct file* filp, char __user *buff, size_t count, loff_t *f_pos)
{
    return 0;
}

ssize_t gpmc_write(struct file* filp, const char __user *buff, size_t count, loff_t *f_pos)
{
    return -ENOMEM;
}

loff_t gpmc_lseek(struct file *filp, loff_t offset, int whence)
{
    return 0;
}
struct file_operations gpmc_fops = {
    .open    = gpmc_open, 
    .write   = gpmc_write,
    .read    = gpmc_read, 
    .llseek  = gpmc_lseek, 
    .release = gpmc_release,
    .owner   = THIS_MODULE
};
struct gpmcdev_platform_data* gpmcdev_get_platdata_from_dt(struct device *dev)
{
    //DT node
    struct device_node *dev_node = dev->of_node; //of_node contains device details
    struct gpmcdev_platform_data *pdata;
    if(!dev_node)
        return NULL;
    
    pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
    if(!pdata) {
        dev_info(dev, "cannot allocate platform data memory\r\n");
        return ERR_PTR(-ENOMEM);
    }

    //store node information to pdata
    if(of_property_read_string(dev_node, "fpga,device-serial-num", &pdata->serial_number)) {
        dev_info(dev, "missing serial number property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32_array(dev_node, "fpga,gpmc-reg", pdata->gpmc_base, ARRAY_SIZE(pdata->gpmc_base))) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "fpga,block-size", &pdata->block_size)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32_array(dev_node, "fpga,gpmc-conf1-n", pdata->gpmc_conf1, ARRAY_SIZE(pdata->gpmc_conf1))) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32_array(dev_node, "fpga,gpmc-conf2-n", pdata->gpmc_conf2, ARRAY_SIZE(pdata->gpmc_conf2))) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32_array(dev_node, "fpga,gpmc-conf3-n", pdata->gpmc_conf3, ARRAY_SIZE(pdata->gpmc_conf3))) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32_array(dev_node, "fpga,gpmc-conf4-n", pdata->gpmc_conf4, ARRAY_SIZE(pdata->gpmc_conf4))) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32_array(dev_node, "fpga,gpmc-conf5-n", pdata->gpmc_conf5, ARRAY_SIZE(pdata->gpmc_conf5))) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32_array(dev_node, "fpga,gpmc-conf6-n", pdata->gpmc_conf6, ARRAY_SIZE(pdata->gpmc_conf6))) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32_array(dev_node, "fpga,gpmc-conf7-n", pdata->gpmc_conf7, ARRAY_SIZE(pdata->gpmc_conf7))) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }
    
    if(of_property_read_u32(dev_node, "fpga,gpmc-cs-num", &pdata->gpmc_cs_num)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32_array(dev_node, "fpga,gpmc-cs", pdata->gpmc_cs, ARRAY_SIZE(pdata->gpmc_cs))) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "fpga,ctrl-reg", &pdata->ctrl_base)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "fpga,ctrl-adn-offset", &pdata->ctrl_base_ad0)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }
    
    if(of_property_read_u32(dev_node, "fpga,ctrl-csn-offset", &pdata->ctrl_base_cs0)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "fpga,ctrl-wen-offset", &pdata->ctrl_base_wen)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "fpga,ctrl-advn-offset", &pdata->ctrl_base_advn)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }

    if(of_property_read_u32(dev_node, "fpga,ctrl-oen-offset", &pdata->ctrl_base_oen)) {
        dev_info(dev, "missing property\r\n");
        return ERR_PTR(-EINVAL);
    }
    return pdata;
}

struct of_device_id gpmc_dt_match[] =
{
    {.compatible = "GPMC-FPGA", .data = (void*)PCDEVA1X}, 
    {}
}; 
/* called when matched platform device is found */
int gpmc_platform_driver_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct gpmcdev_private_data *dev_data;
    struct gpmcdev_platform_data *pdata;
    int driver_data;
    int i, ret;
    pr_info("GPMC device detected\r\n");
    dev_info(dev, "A device is detected\r\n");
    pdata = gpmcdev_get_platdata_from_dt(dev);

    if(IS_ERR(pdata))
        return -EINVAL;
    if(!pdata) {
        pdata = (struct gpmcdev_platform_data*)dev_get_platdata(dev);
        if(!pdata) {
            dev_info(dev, "No platform data available\n");
            return -EINVAL;
        }
        driver_data = pdev->id_entry->driver_data;
    } else {
        driver_data = (int)of_device_get_match_data(dev);
    }

    /* allocate memory for device private data */
    dev_data = devm_kzalloc(dev, sizeof(struct gpmcdev_private_data), GFP_KERNEL);
    if(!dev_data) {
        pr_info("Cannot allocate memory\r\n");
        return -ENOMEM;
    }

    dev_data->pdata.serial_number = pdata->serial_number;
    gpmcdrv_data.gpmcdev_data.pdata.serial_number = pdata->serial_number;
    pr_info("GPMC device serial number %s\n", dev_data->pdata.serial_number);
    
    for (i = 0; i < 2; i++) {
        gpmcdrv_data.gpmcdev_data.pdata.gpmc_base[i] = dev_data->pdata.gpmc_base[i] = pdata->gpmc_base[i];
    }
    pr_info("GPMC register base %x size %x\n", dev_data->pdata.gpmc_base[0], dev_data->pdata.gpmc_base[1]);
    
    gpmcdrv_data.gpmcdev_data.pdata.gpmc_sys_config = dev_data->pdata.gpmc_sys_config = pdata->gpmc_sys_config;
    pr_info("GPMC sys config offset %x\n",  dev_data->pdata.gpmc_sys_config);

    for(i = 0; i < 2; i++) {
        gpmcdrv_data.gpmcdev_data.pdata.gpmc_conf1[i] = dev_data->pdata.gpmc_conf1[i] = pdata->gpmc_conf1[i];
    }
    pr_info("GPMC config_1 offset %x offset %x\n", dev_data->pdata.gpmc_conf1[0], dev_data->pdata.gpmc_conf1[1]);
    
    gpmcdrv_data.gpmcdev_data.pdata.block_size = dev_data->pdata.block_size = pdata->block_size;

    gpmcdrv_data.gpmcdev_data.pdata.gpmc_cs_num = dev_data->pdata.gpmc_cs_num = pdata->gpmc_cs_num;
    pr_info("GPMC cs num %d\n",  dev_data->pdata.block_size);

    for(i = 0; i < dev_data->pdata.gpmc_cs_num; i++) {
        gpmcdrv_data.gpmcdev_data.pdata.gpmc_cs[i] = dev_data->pdata.gpmc_cs[i] = pdata->gpmc_cs[i];
        pr_info("GPMC cs%d %d\n",i,  dev_data->pdata.gpmc_cs[i]);
    }

    gpmcdrv_data.gpmcdev_data.pdata.ctrl_base = dev_data->pdata.ctrl_base = pdata->ctrl_base;
    pr_info("CTRL base %x\n",  dev_data->pdata.ctrl_base);

    gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_ad0 = dev_data->pdata.ctrl_base_ad0 = pdata->ctrl_base_ad0;
    pr_info("CTRL ad0 offset %x\n",  dev_data->pdata.ctrl_base_ad0);

    gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_cs0 = dev_data->pdata.ctrl_base_cs0 = pdata->ctrl_base_cs0;
    pr_info("CTRL cs0 offset %x\n",  dev_data->pdata.ctrl_base_cs0);

    gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_wen = dev_data->pdata.ctrl_base_wen = pdata->ctrl_base_wen;
    pr_info("CTRL wen offset %x\n",  dev_data->pdata.ctrl_base_wen);

    gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_advn = dev_data->pdata.ctrl_base_advn = pdata->ctrl_base_advn;
    pr_info("CTRL advn offset %x\n",  dev_data->pdata.ctrl_base_advn);

    gpmcdrv_data.gpmcdev_data.pdata.ctrl_base_oen = dev_data->pdata.ctrl_base_oen = pdata->ctrl_base_oen;
    pr_info("CTRL one offset %x\n",  dev_data->pdata.ctrl_base_oen);

    dev_data->buffer = devm_kzalloc(dev, dev_data->pdata.block_size, GFP_KERNEL);
    if(!dev_data->buffer) {
        pr_info("Cannot allocate memory\r\n");
        return -ENOMEM;
    }

    /* get device number */
    dev_data->dev_num = gpmcdrv_data.device_num_base + gpmcdrv_data.total_devices;

    /* cdev init */
    cdev_init(&dev_data->cdev, &gpmc_fops);
    dev_data->cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
    if(ret < 0) {
        pr_err("cdev add fail \r\n");
        return ret;
    }

    gpmcdrv_data.device_gpmc = device_create(gpmcdrv_data.class_gpmc, dev, dev_data->dev_num, NULL, "gpmc-%d", gpmcdrv_data.total_devices + 1);
    /* create device file for detected device from platform device driver */
    //pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, dev_data->dev_num, NULL, "pcdev-%d", pdev->id);
    if(IS_ERR(gpmcdrv_data.device_gpmc)) {
        pr_err("device creation fail\n");
        ret = PTR_ERR(gpmcdrv_data.device_gpmc);
        cdev_del(&dev_data->cdev);
        return ret;
    }

    gpmcdrv_data.total_devices++;
    pr_info("pcd device is probed\n");
    /* save device private data pointer in platform device structure 
       so can be used later
    */
    dev_set_drvdata(dev, dev_data);
    return 0;
}

int gpmc_platform_driver_remove(struct platform_device *pdev)
{
    struct gpmcdev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
    pr_info("pcd device is removed\n");
    uninit_gpmc_bases();
    /* remove device created */
    device_destroy(gpmcdrv_data.class_gpmc, dev_data->dev_num);
    /* remove cdev entry */
    cdev_del(&dev_data->cdev);
    gpmcdrv_data.total_devices--;
    dev_info(&pdev->dev, "device is removed\r\n");
    return 0;
}

/* matching multiple platform device with different names */
struct platform_device_id pcdev_ids[] = {
    /* driver data is linked to pcdev_config */
    [0] = {.name = "pcdev_A1x", .driver_data = PCDEVA1X}, 
    [1] = {.name = "pcdev_B1x", .driver_data = PCDEVB1X}, 
    [2] = {.name = "pcdev_C1x", .driver_data = PCDEVC1X}, 
    [3] = {.name = "pcdev_D1x", .driver_data = PCDEVD1X}, 
};

struct platform_driver gpmc_platform_driver = 
{
    .probe  = gpmc_platform_driver_probe, 
    .remove =  gpmc_platform_driver_remove, 
    .id_table = pcdev_ids, 
    /* id table is used then name field is no longer valid */
    .driver = {
        .name = "gpmc-char-device", //used to match with platform device
        .of_match_table = of_match_ptr(gpmc_dt_match) //return null if config_of is not defined
                                                           //return table if defined
    }
};

#define MAX_DEVICES 10
static int __init gpmc_plaftform_driver_init(void)
{
    int ret;
    printk("%s: Open: module opened\n",__func__);
    /* dynamic device number allocation */
    ret = alloc_chrdev_region( &gpmcdrv_data.device_num_base,0 , MAX_DEVICES, "gpmcdev");
    if(ret < 0) {
        pr_err("Alloc chr dev fail\r\n");
        return ret;
    }
    gpmcdrv_data.total_devices = 0;
    /* create device class under sys/class */
    gpmcdrv_data.class_gpmc = class_create(THIS_MODULE, "gpmc_class");
    if(IS_ERR(gpmcdrv_data.class_gpmc)) {
        pr_err("Class creation fail\n");
        //convert pointer to error code
        ret = PTR_ERR(gpmcdrv_data.class_gpmc);
        unregister_chrdev_region(gpmcdrv_data.device_num_base, MAX_DEVICES);
        return ret;
    }
    /* register platform driver */
    platform_driver_register(&gpmc_platform_driver);

    pr_info("pcd platform driver loaded\n");
    return 0;
}

static void __exit gpmc_plaftform_driver_cleanup(void)
{
    platform_driver_unregister(&gpmc_platform_driver);

    /* destory class */
    class_destroy(gpmcdrv_data.class_gpmc);
    
    /* unregister device number */
    unregister_chrdev_region(gpmcdrv_data.device_num_base, MAX_DEVICES);
    pr_info("gpmc platform driver cleanup\n");
}

module_init(gpmc_plaftform_driver_init);
module_exit(gpmc_plaftform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LEN");
MODULE_DESCRIPTION("GPMC memory driver");