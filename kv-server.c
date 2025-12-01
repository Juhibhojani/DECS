#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

void error(char *msg)
{
  perror(msg);
  exit(1);
}

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 4 // maximum only 4 arguments are allowed
#define MAX_CWD_SIZE 256
#define NUM_OF_THREADS 2
#define PC_QUEUE 2

/* Splits the string by space and returns the array of tokens
 *
 */
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for (i = 0; i < strlen(line); i++)
  {

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t')
    {
      token[tokenIndex] = '\0';
      if (tokenIndex != 0)
      {
        tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
        strcpy(tokens[tokenNo++], token);
        tokenIndex = 0;
      }
    }
    else
    {
      token[tokenIndex++] = readChar;
    }
  }

  free(token);
  tokens[tokenNo] = NULL;
  return tokens;
}

// variables to store key-value pair
#define MAX_KEY_VALUES 1000
char *key_value_pairs[MAX_KEY_VALUES];             // stores the dynamically allocated key-value pairs
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  // lock used for protecting key-value server
pthread_mutex_t lockq = PTHREAD_MUTEX_INITIALIZER; // Lock used for proctecting queue
int count = 0;                                     // stores the count of value items in buffer
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;      // condition variable to wake up thread when at item is added
pthread_cond_t pcv = PTHREAD_COND_INITIALIZER; // condition variable to wake up producer when buffer becomes empty
int pc_queue[PC_QUEUE] = {0};

int create(int key, int value_size, char *value)
{
  if (key_value_pairs[key] != NULL)
  {
    // return error as key already exists
    return -1;
  }
  else
  {
    // dynamically allocate memory of size value_size
    char *valueStore = (char *)malloc((value_size + 1) * sizeof(char));
    if (valueStore == NULL)
      return 0; // in case error during malloc
    strncpy(valueStore, value, value_size);
    valueStore[value_size] = '\0';
    key_value_pairs[key] = valueStore;
    return 1;
  }
}

// read operation
char *readKey(int key)
{
  return key_value_pairs[key];
}

// update operation
int update(int key, int value_size, char *value)
{
  if (key_value_pairs[key] != NULL)
  {
    // Key already exists
    // free existing memory and create new
    free(key_value_pairs[key]);
    // dynamically allocate memory
    char *valueStore = (char *)malloc((value_size + 1) * sizeof(char));
    if (valueStore == NULL)
    {
      return 0; // in case error during malloc
    }
    strncpy(valueStore, value, value_size);
    valueStore[value_size] = '\0'; // ensures null string termination
    key_value_pairs[key] = valueStore;
    return 1;
  }
  return -1;
}

// delete operation
int delete(int key)
{
  if (key_value_pairs[key] == NULL)
  {
    return -1;
  }
  else
  {
    free(key_value_pairs[key]);
    key_value_pairs[key] = NULL;
    return 1;
  }
}

// a function pointer which takes newsockfd as argument
void *worker_code(void *thread_arg)
{
  // type cast newsockfd to int type
  int thread_num = *(int *)thread_arg; // typcasts given pointer and then saves it's value
  printf("Thread with %d id is created and initialized!", thread_num);
  while (1)
  {
    int newsockfd;
    pthread_mutex_lock(&lockq);
    while (count == 0)
    {
      // sleep then
      printf("Thread %d waiting", thread_num);
      pthread_cond_wait(&cv, &lockq);
    }
    // if a client is ready
    for (int i = 0; i < PC_QUEUE; i++)
    {
      if (pc_queue[i] != 0)
      {
        // let's take this client
        newsockfd = pc_queue[i];
        pc_queue[i] = 0;
        count--;
        break;
      }
    }
    pthread_cond_signal(&pcv);
    pthread_mutex_unlock(&lockq);

    printf("Client handled by thread: %d has sockfd  %d", thread_num, newsockfd);
    int n1, n2;
    /* read message from client */
    char line[MAX_INPUT_SIZE];
    char **tokens;
    do
    {
      bzero(line, MAX_INPUT_SIZE);
      n1 = read(newsockfd, line, MAX_INPUT_SIZE); // reads from socket on client side, blocking system call
      if (n1 < 0)
        error("ERROR reading from socket");
      if (n1 == 0)
      {
        printf("Client is now disconnected! \n");
        break;
      }
      line[strlen(line)] = '\n'; // terminate with new line
      printf("Here is the message: %s\n", line);
      tokens = tokenize(line);

      char msg[256]; // variable to store the message to be sent to user in return
      bzero(msg, 256);

      if (atoi(tokens[1]) > 1000)
      {
        strcpy(msg, "Server doesn't have space!");
      }
      else
      {
        pthread_mutex_lock(&lock);
        // logic to create/read/update/delete key!
        if (strcmp(tokens[0], "create") == 0)
        {
          int key = atoi(tokens[1]);
          int key_value = atoi(tokens[2]);
          // tokens[3] is the value
          int success = create(key, key_value, tokens[3]);
          if (success == -1)
          {
            strcpy(msg, "Given key already exists");
          }
          else if (success == 0)
          {
            strcpy(msg, "Error during creation, please try again later");
          }
          else
          {
            strcpy(msg, "Successfully created key");
          }
        }
        else if (strcmp(tokens[0], "read") == 0)
        {
          int key = atoi(tokens[1]);
          char *value_pointer = readKey(key);
          if (value_pointer == NULL)
          {
            strcpy(msg, "No such key exists");
          }
          else
          {
            strcpy(msg, value_pointer);
          }
        }
        else if (strcmp(tokens[0], "update") == 0)
        {
          int key = atoi(tokens[1]);
          int key_value = atoi(tokens[2]);
          // tokens[3] is the value
          int success = update(key, key_value, tokens[3]);
          if (success == -1)
          {
            strcpy(msg, "Given key doesn't exists, please create first!");
          }
          else if (success == 0)
          {
            strcpy(msg, "Error during updation, please try again later");
          }
          else
          {
            strcpy(msg, "Successfully updated key");
          }
        }
        else if (strcmp(tokens[0], "delete") == 0)
        {
          int key = atoi(tokens[1]);
          int success = delete(key);
          if (success == -1)
          {
            strcpy(msg, "No such key exists");
          }
          else
          {
            strcpy(msg, "Successfully deleted!");
          }
        }
        pthread_mutex_unlock(&lock);
      }
      /* send reply to client */
      n2 = write(newsockfd, msg, strlen(msg));
      if (n2 < 0)
        error("ERROR writing to socket");
    } while (n1 > 0 && n2 > 0);
    close(newsockfd);
  }
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n1, n2;
  if (argc < 3)
  {
    fprintf(stderr, "ERROR, no port or IP address provided\n");
    exit(1);
  }

  /* create socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0); // creates a socket using internet protocol and TCP type

  if (sockfd < 0)
    error("ERROR opening socket");

  /* fill in port number to listen on. IP address can be anything (INADDR_ANY) */
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[2]);
  serv_addr.sin_family = AF_INET;
  printf("%s \n", argv[1]);
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // needs to be inet_addr("127.0.0.1") i.e. string being parsed as ip address
  serv_addr.sin_port = htons(portno);

  /* bind socket to this port number on this machine */
  if (bind(sockfd, (struct sockaddr *)&serv_addr,
           sizeof(serv_addr)) < 0)
    error("ERROR on binding"); // bind the server socket with given port number

  /* listen for incoming connection requests */
  listen(sockfd, 5); // listen to incoming connections on sockfd FD and 5 is the maximum length of incoming connection queue maintained
  clilen = sizeof(cli_addr);

  // create a multiple new threads here
  pthread_t thread_id[NUM_OF_THREADS];
  for (int i = 0; i < NUM_OF_THREADS; i++)
  {
    int *thread_num = (int *)malloc(sizeof(int));
    *thread_num = i;
    pthread_create(&thread_id[i], NULL, worker_code, thread_num);
    pthread_detach(thread_id[i]);
  }
  while (1)
  {
    pthread_mutex_lock(&lockq);
    
    while(count==PC_QUEUE){
      // reached the max capacity
      printf("reached max capacity \n");
      pthread_cond_wait(&pcv,&lockq);
    }
    pthread_mutex_unlock(&lockq);

    /* accept a new request, create a newsockfd */
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen); // accepts the connection, one of them from queue on listen and returns the address of peer connected via it
    if (newsockfd < 0)
      error("ERROR on accept");

    printf("New client accepted, now lets add it to buffer! \n");

    pthread_mutex_lock(&lockq);
    // add this new client to the queue
    for(int i=0;i<PC_QUEUE;i++){
      if(pc_queue[i]==0){
        pc_queue[i]=newsockfd;
        count++;
        break;
      }
    }
    pthread_cond_signal(&cv); // always signal before unlocking
    pthread_mutex_unlock(&lockq);
    printf("gave work to a new thread, now will move on to accepting new client\n");
  }
  return 0;
}