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


int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;


	while(1) {			
		/* BEGIN: TAKING INPUT */
		char buf[MAX_CWD_SIZE];
		getcwd(buf,MAX_CWD_SIZE);
		bzero(line, sizeof(line));
		printf("%s$ ",buf);
		scanf("%[^\n]", line);
		getchar();

		// printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);
		
		if(tokens[0]==NULL){
			// no command is passed and we continue taking input
			continue;
		}

		// tokens[0] is the command to be executed and rest is the arguments for that command

		// for(i=0;tokens[i]!=NULL;i++){
		// 	printf("found token %s (remove this debug output later)\n", tokens[i]);
		// }
		// in case i want to change the working directory, i won't fork a new process
		char *s1 = "cd";
		if(strcmp(tokens[0],s1)==0){
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
		}
		else{
			// forking a child process
			int rc = fork();
			if(rc<0){
				printf("Fork fails, shell failed \n");
				exit(1);
			}
			else if(rc==0){
				// changing the executable here
				int exitStatus = execvp(tokens[0],tokens);
				// exec returns -1 in case the command executed doesn't exists
				// in case the exec command fails because the command to be executed is incorrect
				printf("No such command exists \n"); 
				exit(exitStatus);
			}
			else{
				// parent process
				// waits for the child process
				int status;
				int wc = wait(&status);
				if(WEXITSTATUS(status)!=0){
					printf("EXIT STATUS: %d \n",WEXITSTATUS(status));
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
