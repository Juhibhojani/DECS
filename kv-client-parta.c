#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>

void error(char *msg)
{
    perror(msg);
    exit(0);
}

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 4 // maximum only 4 arguments are allowed
#define MAX_CWD_SIZE 256

/* Splits the string by space and returns the array of tokens
 *
 */


FILE *fp; // file pointer

void free_tokens()
{
    for (int i = 0; tokens[i] != NULL; i++)
    {
        free(tokens[i]);
    }
    free(tokens);
}

// int portno = 5000;
// char hostname[] = "localhost";

int main(int argc, char *argv[])
{
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 2)
    {
        fprintf(stderr, "usage %s mode file-name[if required]\n", argv[0]);
        exit(0);
    }
    else if (strcmp(argv[1], "interactive") != 0 && strcmp(argv[1], "batch") != 0)
    {
        printf("Mode can either be batch or interactive \n");
    }
    else if (strcmp(argv[1], "batch") == 0)
    {
        if (argc < 3)
        {
            error("Sorry, you must provide filename to work in batch mode! \n");
        }
        fp = fopen(argv[2], "r");
        if (fp == NULL)
        {
            error("Unable to open given file!");
        }
        printf("file opened! \n");
    }

    int mode = strcmp(argv[1], "interactive") == 0 ? 1 : 0; // mode is 1 if interactive and 0 if batch

    int active_connection = 0;

    while (1)
    {
        bzero(line, sizeof(line));
        if(mode==1){
            printf("reading inputs from user \n");
            scanf("%[^\n]", line);
            getchar();
            line[strlen(line)] = '\n'; // terminate with new line

        }
        else{
            char* newLine = fgets(line, MAX_INPUT_SIZE, fp);
            if(newLine==NULL){
                return 0; // can terminate due to error or when file is empty
            }
            printf("line %s \n",line);
            line[strlen(line)] = '\n'; // terminate with new line
        }
        // printf("before tokenization \n");
        tokens = tokenize(line);
        if(tokens[0]==NULL){
            continue; // no input enter
        }
        // printf("tokens[0] : %s \n",tokens[0]);

        // handling connect commad
        if (strcmp(tokens[0], "connect") == 0)
        {
            // only start to make a connection if there is active connection
            if (active_connection)
            {
                printf("Sorry client can have only one active connection at a time \n");
            }
            // make a connection
            /* create socket, get sockfd handle */

            portno = atoi(tokens[2]);
            sockfd = socket(AF_INET, SOCK_STREAM, 0); // creating a new socket
            if (sockfd < 0)
                printf("ERROR opening socket");

            /* fill in server address in sockaddr_in datastructure */

            bzero((char *)&serv_addr, sizeof(serv_addr)); // initializing serv_address with 0's
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = inet_addr(tokens[1]);
            serv_addr.sin_port = htons(portno);

            /* connect to server */

            if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ // connect ot server using my sockfd(my socket) and server and client address
                printf("ERROR connecting \n");
                continue;
            }

            active_connection = 1;
            printf("OK \n");
        }
        // handling disconnect command
        else if (strcmp(tokens[0], "disconnect") == 0)
        {
            if (!active_connection)
            {
                printf("No active connection exists to disconnect it");
                continue;
            }
            else
            {
                // disconnect from the server
                int closeNum = close(sockfd);
                if (closeNum == 0)
                {
                    active_connection = 0;
                    printf("OK disconnected \n");
                }
                else
                {
                    printf("Some problem while disconnecting user, sorry! Try again \n");
                }
            }
        }
        // handling create command
        else if ((strcmp(tokens[0], "create") == 0) || (strcmp(tokens[0], "read")) == 0 || (strcmp(tokens[0], "delete")) == 0 || (strcmp(tokens[0], "update")) == 0)
        {
            // return error if no active connection exists
            if (!active_connection)
            {
                printf("Sorry, no active connection exists, please first try connecting! \n");
                continue;
            }
            // let's check if number of arguments provided are correct or not
            if ((strcmp(tokens[0], "create") == 0) || (strcmp(tokens[0], "update") == 0))
            {
                if (tokens[3] == NULL)
                {
                    printf("Incorrect number of arguments provided! \n");
                    continue;
                }
                if(atoi(tokens[2])!=strlen(tokens[3])){
                    printf("incorrect size of value, please try again\n");
                    continue;
                }
            }
            if ((strcmp(tokens[0], "delete") == 0) || (strcmp(tokens[0], "read") == 0))
            {
                if (tokens[1] == NULL)
                {
                    printf("Incorrect number of arguments provided! \n");
                    continue;
                }
            }
            // send this key value to the server
            // sending the entire line buffer
            n = write(sockfd, line, strlen(line));
            if (n < 0)
                error("ERROR writing to socket");

            // let's wait for client to send the response back
            bzero(buffer, 256);
            n = read(sockfd, buffer, 256);
            if (n < 0)
                error("ERROR reading from socket");
            printf("%s\n", buffer);
        }
        else
        {
            error("Error: Incorrect command! \n");
        }
        free_tokens(tokens); // freeing up memory
    }
    return 0;
}