#include <stdio.h> // for printf
#include <unistd.h> // for any linux utilities
#include <signal.h> // for signal
#include <sys/wait.h> // for wait() and fork
#include <string.h> // for memset
#include <stdlib.h> // for exit
#include <sys/types.h> // for kill
#include <stdbool.h> // for bool

#define LENGTH 8 // The fixed length bitstring

char recvdString[9]; // the buffer for the child to store the received string into
bool canSend = false; // synchronization mechanism for notifying the parent that the child is ready to receive
int i=0;

void sigHandle1(int sig) { 
    recvdString[i] = '1';
    // printf("recieved string at index %d %d \n",i,recvdString[i]);
    i++;
    // sending signal to parent to say that it's ready to communicate
    kill(getppid(),SIGUSR1);
}

void sigHandle0(int sig) {
    recvdString[i] = '0';
    // printf("recieved string at index %d %d \n",i,recvdString[i]);
    i++;
    // sending signal to parent to say that it's ready to communicate
    kill(getppid(),SIGUSR1);
}

void synchronizeParent (int sig) { // helper function to ensure the parent only sends once the child is ready to receive
    canSend = true;
}

int main () {

    signal(SIGUSR1,synchronizeParent); // to ensure parent is able to understand when child is telling it that its ready to receive
    int cpid = fork(); // fork
    if (cpid == 0) {
        signal(SIGUSR1,sigHandle1);
        signal(SIGUSR2,sigHandle0);
        kill(getppid(),SIGUSR1); // Sends signal to parent that child is ready to receive
        while(i!=8){;}
        recvdString[8] = '\0'; // Null terminates the string so that it prints in an expected manner
        printf("[Child] Received bitstring is\t%s\n",recvdString); // Do not edit, prints the received bitstring

        exit(0);
    }
    else {

        printf("Please input a %d-bit bitstring:\t",LENGTH);

        char tmp[256]; // buffer to store bitstring
        fgets(tmp,LENGTH + 1,stdin); // Take input from user

        if (strlen(tmp) != 8) {
            printf("Error : Input string not of length %d\n",LENGTH);
            kill(cpid,9);
            wait(NULL);
            exit(1);
        }

        for (int i=0; i < LENGTH; i++) {
            if (!(tmp[i] == '1' || tmp[i] == '0')) {
                printf("Error : Input string not a bitstring at index %d char %c\n",i,tmp[i]);
                kill(cpid,9);
                wait(NULL);
                exit(1);
            }
        }

        printf("[Parent] Input bitstring is \t%s\n",tmp);

        while (!canSend) {;} // Wait until the child is ready to receive
        for (int i=0; i < LENGTH; i++) {
            while(!canSend){;}
                if (tmp[i] == '1') {
                    kill(cpid,SIGUSR1);
                    canSend=false;
                }
                else {
                    kill(cpid,SIGUSR2);
                    canSend=false;
                }
                
            // sleep(1); // This is necessary to ensure the signals 1]all get sent to child and 2] are sent in correct ordering, increase time to increase reliability
        }
        wait(NULL); // reap the child
    }

}