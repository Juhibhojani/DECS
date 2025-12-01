#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h> 
# include<sys/stat.h>

#define SOCK_PATH "/tmp/my_unix_socket"

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n; 

    struct sockaddr_un serv_addr;
    struct stat buffer_fs;
    char buffer[256];

    /* create socket, get sockfd handle */
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    // socket created

    /* fill in server address */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, SOCK_PATH); 

    // in this case the data to be sent is the file in user input
    if(argc!=2){
        printf("Please provide file name as input! \n");
        exit(1);
    }

    // trying to open the file
    int fd = open(argv[1],O_RDONLY);
    if(fd<0){
        printf("File doesn't exists or error reading the file \n");
    }

    // trying to get information regarding file
    int rc_fstat = fstat(fd,&buffer_fs);
    printf("rc_fstat: %d \n",rc_fstat);
    printf("Buffer value %ld \n",buffer_fs.st_size); // getting the size file
    int size = buffer_fs.st_size;    

    /* ask user for input */
    // off_t filesize = buffer_fs.st_size;

    // send file size first
    if (sendto(sockfd, &size, sizeof(size), 0,
               (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR sending filesize");
    }


    // now let's read the file
    // defining buffer for reading 256 bytes size of data
    while(1)
    {
        // cleaning the buffer
        bzero(buffer,256);
        // reading the file in chunks of 256
        int rc = read(fd,buffer,256);
        if(rc<=0){
            printf("done with reading\n");
            break;
        }
        /* send user message to server */
        printf("Sending data...\n");
        n = sendto(sockfd, buffer, rc, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (n < 0) {
            error("ERROR writing to socket");
            break;
        }
    }
    // closing the file descriptor
    close(fd);
    // close the socket file descriptor
    close(sockfd);

    return 0;
}
