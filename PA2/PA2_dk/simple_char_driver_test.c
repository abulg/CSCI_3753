#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>

#define DEVICE "/dev/simple_char_driver"
#define BUFFER_SIZE 1024



int main() {
    char cmd;
    char bufftosend[BUFFER_SIZE] = {'\0'};
    int readret;
    int writeret;

    int read_buffer_size = 0;
    int offset = 0;
    int whence = 0;

    int file = open(DEVICE, O_RDWR);
    if(file < 0) {
        printf("Failed to open device\n");
        return -1;
    }

    bool terminal = false;

    while(!terminal) {
        printf("r) Read\nw) Write\ns) Seek\ne) Exit\n");
        scanf("%c", &cmd);
        switch(cmd) {
        case 'w': {
            printf("Enter String: ");
            scanf("%s", bufftosend);
            while(getchar() != '\n');   //cleans input stream for next input

            printf("Writing message to device [%s]\n", bufftosend);
            writeret = write(file, bufftosend, strlen(bufftosend));

            if(writeret < 0) {
                printf("Failed to write to device\n");
                return -2;
            } else {
                printf("Wrote %d bytes to device file\n", writeret);
            }

            break;
        }
        case 'r': {
            printf("Enter number of bytes you want to read: ");
            scanf("%d", &read_buffer_size);
            while(getchar() != '\n');

            if(read_buffer_size > BUFFER_SIZE - 1){
                printf("Number of bytes to read is greater than the buffer size\n");
                break;
            }

            char *bufftoreceive = (char *)malloc(read_buffer_size * sizeof(char));  //allocate correct amount of memory to the read buffer
            readret = read(file, bufftoreceive, read_buffer_size);

            if(readret < 0) {
                printf("Failed to read from device\n");
                return -3;
            } else {
                printf("Data read from device: [%s]\n", bufftoreceive);
            }

            free(bufftoreceive);
            break;
        }
        case 's': {
            printf("Enter offset value: ");
            scanf("%d", &offset);
            while(getchar() != '\n');

            if(offset < (-writeret + 1) || (offset > writeret - 1)) {
                printf("You are seeking beyond the file size\n");
                break;
            }

            printf("Enter whence value: ");
            scanf("%d", &whence);
            while(getchar() != '\n');

            if(!(whence >= 0 && whence < 3)) {
                printf("Enter correct whence values\n");
                break;
            }

            lseek(file, offset, whence);
            break;
        }
        case 'e': {
            printf("Exiting\n");
            terminal = true;
            break;
        }
        default: {

        }
        }
    }
    close(file);
    return 0;
}
