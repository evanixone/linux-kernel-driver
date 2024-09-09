#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>

int displayInit()
{
    int fd, retval;
    char buf[100];
    int ret;
    //Open DISPLAY_DEVICE
    fd = open("/dev/display_device", O_RDWR);
    if (fd < 0)
    {
        printf("Error opening device\n");
        return -1;
    }
        
    //Write DISPLAY_DEVICE
    strcpy(buf, "\nTEMPERATURE :10.00");
    ret = write(fd, buf, strlen(buf) + 1);
    
    close(fd);
    return 1;
}

int main()
{
    int fd, retval;
    char buf[100];
    char buf2[100];
    int ret;
    displayInit();
    
    while(1)
    {
        /**
         * TEMP_DEVICE
         **/
         
        //Open TEMP_DEVICE
        fd = open("/dev/temp_device", O_RDWR);
        if (fd < 0)
        {
            printf("Error opening device\n");
            return -1;
        }
        
        //Read TEMP_DEVICE
        retval = read(fd, buf2, 100);
        if (retval < 0)
        {
            perror("Failed to read the message from the device...");
            close(fd);
            exit(1);
        }
        printf("Received Data: \n");
        printf("%s \n", buf2);
        
        //Close TEMP_DEVICE
        close(fd);
        
        /**
         * DISPLAY_DEVICE
         **/
        
        //Open DISPLAY_DEVICE
        fd = open("/dev/display_device", O_RDWR);
        if (fd < 0)
        {
            printf("Error opening device\n");
            return -1;
        }
        
        //Write DISPLAY_DEVICE
        strcpy(buf, "\b\b\b\b\b");
        ret = write(fd, buf, strlen(buf) + 1);
        ret = write(fd, buf2, strlen(buf) + 1);
        if (ret < 0)
        {
            printf("Error writing to device\n");
            return -1;
        }
        
        //Close DISPLAY_DEVICE
        close(fd);
        
        sleep(1);
    }
    return 0;
}
