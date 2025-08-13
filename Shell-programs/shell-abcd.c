#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_CWD_SIZE 256
#define MAX_BG_PROCESS_SIZE 64

int foreground_process_pid;

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for(i =0; i < strlen(line); i++){

        char readChar = line[i];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
        token[tokenIndex] = '\0';
        if (tokenIndex != 0){
        tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
        strcpy(tokens[tokenNo++], token);
        tokenIndex = 0; 
        }
        } else {
        token[tokenIndex++] = readChar;
        }
    }
    
    free(token);
    tokens[tokenNo] = NULL ;
    return tokens;
}

void free_tokens(char **tokens){
    // Freeing the allocated memory	
    for(int i=0;tokens[i]!=NULL;i++){
        free(tokens[i]);
    }
    free(tokens);
    return;
}

// code for defining signal handler for SIGINT handler
void signalHandler(int sig)  { 
    // this should only kill the foreground process and not the background process
    printf("Caught signal SIGINT, now gotta kill foreground process\n");
    if(foreground_process_pid!=0){
        kill(foreground_process_pid,SIGKILL);
        waitpid(foreground_process_pid,NULL,0);
        printf("foreground process killed here! \n");
		foreground_process_pid=0;
    }
    
} 

int main(int argc, char* argv[]) {
    // code to overwrite signal handler
    signal(SIGINT,signalHandler);
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	// initialize an array with 0 values for storing pid of background processes
	int background_processes[MAX_BG_PROCESS_SIZE] = {0};
	int bg_count=0;

	while(1) {
		/* BEGIN: TAKING INPUT */
		char buf[MAX_CWD_SIZE];
        // gets the current working directory
		getcwd(buf,MAX_CWD_SIZE);
		bzero(line, sizeof(line));
		
		// checking for background process here
		if(bg_count>0){
			for(int i=0;i<MAX_BG_PROCESS_SIZE;i++){
				if(background_processes[i]!=0){
					// checking if this child process has completed execution or not
					int stat=0;
					int cpid = (int)waitpid(background_processes[i],&stat,WNOHANG);
					// check status only if we are getting cpid i.e. the process completed execution
                    // wait pid returns 3 values: PID of child if exited, 0 if none of the child exited and -1 if there is something wrong during the execution of waitpid
					if(cpid>0){
                        bg_count--;
                        background_processes[i]=0;
                        printf("Background child process %d terminated with status: %d\n", cpid, WEXITSTATUS(stat));
					}
				}
			}
		}
		printf("%s$ ",buf);
		scanf("%[^\n]", line);
		getchar();

		// printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

        // for(i=0;tokens[i]!=NULL;i++){
        // 	printf("found token %s (remove this debug output later)\n", tokens[i]);
        // }

		if(tokens[0]==NULL){
			// no command is passed and we continue taking input
            free_tokens(tokens);
			continue;
		}

		// here size will help me in finding the last token
        int size = 0;
        while(tokens[size]!=NULL){
            size++;
        }

        // check if process is background or foreground
		char *s2 = "&";
		int is_background = strcmp(tokens[size-1],s2)==0;
		if(is_background==1){
			if(bg_count==MAX_BG_PROCESS_SIZE){
					printf("Sorry no more than %d processes can run in background, please try again later! \n",MAX_BG_PROCESS_SIZE);
                    free_tokens(tokens);
					continue;
			}
			tokens[size-1]=NULL;
		}

        char *s3 = "exit";

		// for cd : in case I want to change the working directory, i won't fork a new process
		char *s1 = "cd";
		if(strcmp(tokens[0],s1)==0){
			if(is_background==1){
				printf("Can't run this process in background, SORRY! \n");
                free_tokens(tokens);
				continue;
			}
			// changing the directory
			if(tokens[1]==NULL){
				printf("Please provide a directory to cd into \n");
			}
			else if(tokens[2]!=NULL){
				printf("Too many arugments provided \n");
			}
			else{
				int error = chdir(tokens[1]);
				if (error != 0) {
					perror("cd"); // prints custom message along with the recently occured error in the system avilable in the stderr
					// it uses the errno which is set by the system call ,when it fails
					// prints "cd: <system error message>" to stderr, based on the most recent error (from errno)
				}
			}
		}else if(strcmp(tokens[0],s3)==0){
            // assuming exit can't run as background process

            if(is_background==1){
				printf("Can't run this process in background, SORRY! \n");
                free_tokens(tokens);
				continue;
			}

            char *exitStatus = tokens[1] ? tokens[1] : "0";
            // handling arguments in exit
            if(tokens[2]!=NULL){
                printf("Only exit code can be given to exit, please write without any extra arguments\n");
                free_tokens(tokens);
                continue;
            }
            else{
                // I will write the code to exit the process here.
                // kill all background process
                for(int i=0;i<MAX_BG_PROCESS_SIZE;i++){
                    if(bg_count==0){
                        printf("All child processes are killed or none existed \n");
                        break;
                    }
                    if(background_processes[i]!=0){
                        // kill this child process
                        int kill_rt = kill(background_processes[i],SIGKILL);
                        // the kill system call only kills the child process but doesn't reap it
                        // need to reap this child process using waitpid
                        waitpid(background_processes[i],NULL,0);
                        printf("Killed child process with pid %d and return value of kill is %d \n",background_processes[i],kill_rt);
                        background_processes[i]=0;
                        bg_count--;
                    }
                }
                // all background processes are killed
				free_tokens(tokens);
				break;
            }
        }
		else{
			// forking a child process
			int rc = fork();
			if(rc<0){
				printf("Fork fails, shell failed \n");
				exit(1);
			}
			else if(rc==0){
				// setting default signal handler in child so it doesn't show weird behaviour 
				signal(SIGINT, SIG_DFL);
				// checking if this process is a background process or not
				if(is_background==1){
                    // changing it's pid
                    setpgid(0,0);
					// this means we will run this process in background
					// changing the executable here
					int exitStatus = execvp(tokens[0],tokens);
					// mimicing shell here and saying no such command exists
					printf("%s : No such command exists  \n",tokens[0]); 
					// this process should exit and the background process will reap this, keeping status -1 for rn
					exit(exitStatus);
				}
				// changing the executable here
				int exitStatus = execvp(tokens[0],tokens);
				// exec returns -1 in case the command executed doesn't exists
				// in case the exec command fails because the command to be executed is incorrect
				printf("%s : No such command exists \n",tokens[0]); 
				exit(exitStatus);
			}
			else{
				// parent process

				// check if the current process is background or foreground
				// only wait if it's a foreground process
				if(is_background==1){
					// adding into the array of background processes
					for(int i=0;i<MAX_BG_PROCESS_SIZE;i++){
						if(background_processes[i]==0){
							background_processes[i] = rc;
							bg_count++;
							break;
						}
					}
				}
                else{
                    // waits for the child process
                    int status;
                    foreground_process_pid = rc;
                    int wc = waitpid(rc,&status,0);
                    if(WEXITSTATUS(status)!=0){
                        printf("EXIT STATUS: %d \n",WEXITSTATUS(status));
                    }
                }
			}
		}
		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);

	}
	return 0;
}
