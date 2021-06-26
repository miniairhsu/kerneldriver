#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>

#define BASEPORT 0x70

int main() 
{
    char val = 0;
    //get access
    if (ioperm(BASEPORT, 4, 1)) {
        perror("ioperm");
        exit(1);
    }
    // read one byte
    val = inb(BASEPORT + 1);
    printf("valu is %x\r\n", val);

    if (ioperm(BASEPORT, 3, 0)) {
        perror("ioperm");
        exit(1);
    }
    return 0;
}