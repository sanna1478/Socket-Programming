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

#define TCP_PORT_MON 25621

#define TRUE 1
#define FALSE 0

#define BUFSIZE 1024

void stopRunning(int signum);
void printResult(char command[], char buf[]);

int keepRunning = TRUE;
int socketfd = -1;
int linkID;

char writecmd[] = "write";
//char computecmd[] = "compute";
char finished[] = "finished";

int main(void) {
  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;

  int clientLen;
  int numBytes;

  char buf[BUFSIZE] = "";
  char command[30];

  //memset(buf,0,sizeof buf);

  signal(SIGINT, stopRunning);

  socketfd = socket(AF_INET, SOCK_STREAM, 0);
  if(socketfd < 0) {
    perror("client: socket");
    exit(1);
  }
  /****************************************************************************
   * The the connection/binding code was inspired from the
   * Beej socket prgramming tutorial
   ****************************************************************************/
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  serverAddr.sin_port = htons(TCP_PORT_MON);

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
  printf("The monitor is up and running\n");
  shutdown(socketfd,SHUT_WR);

  while(keepRunning == TRUE) {
    numBytes = recv(socketfd,buf,BUFSIZE-1,0);
    if(numBytes < 0) {
      perror("client: recv");
    } else if (numBytes > 0) {
      buf[numBytes] = '\0';
      sscanf(buf,"%s",command);
      printResult(command, buf);
    }
  }
  close(socketfd);
}

void stopRunning(int signum) {
  keepRunning = FALSE;
  if(socketfd != -1) {
    close(socketfd);
  }
  printf("\n");
  exit(0);
}

void printResult(char command[], char buf[]) {
  if(strcmp(command, "write") == 0) {
    char title[30];
    double bw;
    double len;
    double vel;
    double power;
    sscanf(buf,"%s %lf %lf %lf %lf",title,&bw,&len,&vel,&power);
    printf("The monitor received input BW = <%.2lf>, L = <%.2lf>, V = <%.2lf>, and P = <%.2lf> from the AWS\n",bw,len,vel,power);
  }
  if(strcmp(command, "compute") == 0) {
    char title[30];

    int size;
    int sigPower;
    sscanf(buf,"%s %i %i %i",title,&linkID,&size,&sigPower);
    printf("The monitor recieved ID = <%i>, size = <%i>, power = <%i> from the AWS\n",linkID,size,sigPower);
  }
  if(strstr(buf, "finished") != NULL) {
    printf("The write operation has been completed successfully\n");
  }
  if(strstr(buf,"success") != NULL) {
    char header[30];
    double Tt;
    double Tp;
    double delay;
    sscanf(buf,"%s %lf %lf %lf",header,&Tt,&Tp,&delay);
    printf("The result for <%i>:\nTt = <%.2g>ms\nTp = <%.2g>ms\nDelay = <%.2g>ms\n",linkID,Tt,Tp,delay);
  }
  if(strstr(buf,"failure") != NULL) {
    printf("Link ID not found\n");
  }
}
