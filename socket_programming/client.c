#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>

#define TRUE 1
#define FALSE -1

#define WRITE 1
#define COMPUTE 2
#define INVALID -1

#define BUFSIZE 1024

#define AWS_TCP_PORT 24621

void toLowerString(char string[]);
void checkCommand(char *argv[], int *command);
void CheckCmdArgs(int argc, char *argv[], int *command);
void parseData(char *argv[], char buf[], int command, int argc);



int isInt(char string[]);
int isZero(char string[]);
int isFloat(char string[]);
int checkWrite(int argc, char *argv[]);
int checkCompute(int argc, char *argv[]);

int main(int argc, char *argv[]) {

  int command = INVALID;
  int socketfd;
  int numBytes;

  int clientLen;

  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;

  char buf[BUFSIZE] = "";
  char recvBuf[BUFSIZE] = "";

  char finished[] = "finished";

  CheckCmdArgs(argc, argv, &command);

  // Parse the argv data into a single string and save into buf
  parseData(argv, buf, command, argc);
  printf("The client is up and running\n");

  /****************************************************************************
   * The connection/binding code was inspired from the
   * Beej socket prgramming tutorial
   ****************************************************************************/
  socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if(socketfd < 0) {
    perror("client: socket");
    exit(1);
  }

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_port = htons(AWS_TCP_PORT);

  clientAddr.sin_family = AF_INET;
  clientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  clientLen = sizeof(clientAddr);
  if(getsockname(socketfd,(struct sockaddr*)&clientAddr,(socklen_t*)&clientLen) < 0) {
    perror("client: getsockname");
  }

  //Bind the dynamically crated port number to the client socket
  if(bind(socketfd,(struct sockaddr*)&clientAddr,sizeof(clientAddr)) < 0) {
    perror("client: bind");
    exit(1);
  }

  // Connect the client to the server
  if(connect(socketfd,(struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
    perror("client: connect");
    exit(1);
  }

   // Send the message to the server
   numBytes = send(socketfd,buf,strlen(buf),0);
   if(numBytes < 0) {
     perror("client: send");
     exit(1);
   }
   if(command == WRITE) {
     printf("The client sent write operations to AWS \n");
   }
   if(command == COMPUTE) {
     printf("The client sent compute ID=<%s>, size=<%s>, and power=<%s> to AWS \n",
            argv[2],argv[3],argv[4]);
   }

   // Print the servers response
   numBytes = recv(socketfd,recvBuf,BUFSIZE-1,0);
   if(numBytes < 0) {
     perror("client: recv");
   } else if(numBytes > 0) {
     recvBuf[numBytes] = '\0';
     if(command == WRITE && strcmp(recvBuf,finished) == 0) {
       printf("The write operation has been completed successfully\n");
     }
     if(command == COMPUTE) {
       if(strstr(recvBuf,"success") != NULL) {
         char header[30];
         double Tt;
         double Tp;
         double delay;
         sscanf(buf,"%s %lf %lf %lf",header,&Tt,&Tp,&delay);
         delay = Tt + Tp;
         printf("the delay for link <%s> is <%.2g>ms\n",argv[2],delay);
       } else {
         printf("Link ID not found\n");
       }
     }
   }
   close(socketfd);

}







void CheckCmdArgs(int argc, char *argv[], int *command) {
  if (argc >= 2) {
    checkCommand(argv, command);
  }
  if(*command == INVALID) {
    printf("usage: ./client write <BW> <LENGTH> <VELOCITY> <NOISEPOWER>\n");
    printf("usage: ./client compute <LINK_ID> <SIZE> <SIGNALPOWER>\n");
    exit(1);
  } else {
    if(*command == WRITE) {
      if(checkWrite(argc, argv) == -1){
        exit(1);
      }
    }
    if(*command == COMPUTE) {
      if(checkCompute(argc, argv) == -1) {
        exit(1);
      }
    }
  }
}

void checkCommand(char *argv[], int *command) {
  toLowerString(argv[1]);
  if(strcmp(argv[1], "write") == 0) {
    *command = WRITE;
  } else if(strcmp(argv[1],"compute") == 0) {
    *command = COMPUTE;
  } else {
    *command = INVALID;
  }
}

void toLowerString(char string[]) {
  for(int idx=0; idx<strlen(string);idx++) {
    string[idx] = tolower(string[idx]);
  }
}

int checkCompute(int argc, char *argv[]) {
  char args[5][30] = {"compute","./client","LINK_ID","SIZE","SIGNALPOWER"};
  if(argc != 5) {
    printf("Error: To few arguments, expected 5 but received %i\n",argc);
    return -1;
  }
  /*
    LINK_ID: must be a postive int
    SIZE: must be a postive int
    SIGNALPOWER: must be a +ve or -ve float
  */
  // Check that LINK_ID and SIZE is valid
  for(int idx=2;idx<argc-1;idx++) {
    if(isInt(argv[idx]) == -1) {
      printf("Error: %s must strictly be a valid integer\n",args[idx]);
      return -1;
    }
    if(idx == 2) {
      if(isZero(argv[idx]) == 1 || argv[idx][0] == '-') {
        printf("Error: LINK_ID must be a postive integer and greater than zero\n");
        return -1;
      }
    }
  }
  // Check SIGNALPOWER
  if(isFloat(argv[argc-1]) == -1) {
    printf("Error: SIGNALPOWER must be a float or integer\n");
    return -1;
  }
  return 1;
}

int checkWrite(int argc, char *argv[]) {
  char args[6][30] = {"write","./client","BW","LENGTH","VELOCITY","NOISE"};
  if(argc != 6) {
    printf("Error: too few arguments, expected 6 but received %i\n",argc);
    return -1;
  }
    /*
      BW: must be a postive float
      LENGTH: must be positive float greater than 0
      VELOCITY: must be positive float greater than 0
      NOISE: can be +ve or -ve float  but cannot equal 0
    */
    for(int idx=2;idx<argc;idx++) {
      if(isFloat(argv[idx]) == -1) {
        printf("Error: %s must be a valid float or integer type\n",args[idx]);
        return -1;
      }
      if(isZero(argv[idx]) == 1){
        printf("Error: %s must be a value greater than zero\n",args[idx]);
        return -1;
      }
      if(idx != 5) {
        if(argv[idx][0] == '-') {
          printf("Error: %s must be a postive float or integer\n",args[idx]);
            return -1;
        }

      }
    }
    return 1;
}

int isFloat(char string[]) {
  char digit;
  int decimalCounter = 0;
  int idx = 0;
  int negative = FALSE;
  digit = string[idx];
  if(digit == '-') {
    idx = 1;
    negative = TRUE;
  }
  for(; idx<strlen(string);idx++) {
    digit = string[idx];

    if(!isdigit(digit)){
      // Check if a decimal point is in number
      if(digit == '.') {
        decimalCounter++;
      } else {
        // Input argument has something other than digits
        return -1;
      }
    }
  }
  // Checking if after negative sign we have digits
  if(negative == TRUE && strlen(string) <= 1) {
    return -1;
  }
  // Checking if we have only one decimal point
  if(decimalCounter != 1 && decimalCounter !=0) {
    return -1;
  }
  return 1;
}

int isInt(char string[]) {
  char digit;
  int idx = 0;
  int negative = FALSE;
  digit = string[idx];
  if(digit == '-') {
    printf("%c",digit);
    idx = 1;
    negative = TRUE;
  }
  for(; idx<strlen(string);idx++) {
    digit = string[idx];
    if(!isdigit(digit)){
      printf("I am at line 264\n");
      return -1;
    }
  }
  // Checking if after negative sign we have digits
  if(negative == TRUE && strlen(string) == 1) {
    return -1;
  }
  return 1;
}

int isZero(char string[]) {
  char digit;
  int idx = 0;
  int decimalCounter = 0;
  if(string[idx] == '-') {
    return -1;
  }
  for(;idx<strlen(string);idx++) {
    digit = string[idx];
    if(!isdigit(digit)) {
      if(digit == '.') {
        decimalCounter++;
      } else {
        return -1;
      }
    } else {
      if(digit != '0') {
        return -1;
      }
    }
  }
  return 1;
}

void parseData(char *argv[], char buf[], int command, int argc) {
  for(int idx=1; idx<argc;idx++) {
    strcat(buf,argv[idx]);
    if(idx != argc-1) {
      strcat(buf," ");
    }
  }
}
