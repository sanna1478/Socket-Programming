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
#include <math.h>

#define UDP_PORT 22621
#define MAXBUFLEN 1024
#define BUFSIZE 1024
#define TRUE 1
#define FALSE 0

int socketfd;
int keepRunning = TRUE;

void stopRunning(int sig_num);

int main(void) {
  int numBytes;
  int clientAddrLen;

  double bw,len,vel,noise,sigpow;
  double theoBitRate;
  double transmissionDelay;
  double propDealy;
  double totalDelay;
  int linkid,size;
  int reslinkid;
  double temp;

  char commandHeader[30];
  char successHeader[30];

  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;

  char buf[BUFSIZE]= "";
  char result[BUFSIZE]= "";

  signal(SIGINT, stopRunning);

  socketfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(socketfd < 0) {
    perror("listener: socket");
    exit(1);
  }

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(UDP_PORT);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  clientAddrLen = sizeof(clientAddr);
  if(bind(socketfd,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) < 0) {
    perror("listener: bind");
    exit(1);
  }
  printf("The Server B is up and running using UDP on port <%i>\n",UDP_PORT);
  /****************************************************************************
   * The while loop and the connection/binding code was inspired from the
   * Beej socket prgramming tutorial
   ****************************************************************************/
  while(keepRunning == TRUE) {
    memset(buf,0,sizeof buf);
    memset(result,0,sizeof result);
    numBytes = recvfrom(socketfd, buf, BUFSIZE-1,0,(struct sockaddr*)&clientAddr,(socklen_t*)&clientAddrLen);
    if(numBytes < 0) {
      perror("serverB: recvfrom");
    }
    buf[numBytes] = '\0';

    // Parse the data
    sscanf(buf,"%s %i %i %lf %s %i %lf %lf %lf %lf",commandHeader,&linkid,&size,&sigpow,successHeader,&reslinkid,&bw,&len,&vel,&noise);
    printf("The Server B received link information: link <%i>, file size <%i>, and signal power <%.2lf>\n",linkid,size,sigpow);


    sigpow = pow(10.0,(sigpow/10.0));
    noise  = pow(10.0,(noise/10.0));

    temp = 1.0 + (sigpow/noise);
    theoBitRate = bw*1000000*(log(temp)/log(2));
    transmissionDelay = ((double)size/theoBitRate)*1000.0;
    propDealy = (len/vel)*1000.0;

    totalDelay = transmissionDelay + propDealy;
    sprintf(result,"%.2g %.2g %.2g",transmissionDelay, propDealy, totalDelay);
    printf("The Server B finished the calculation for link <%i>\n",linkid);
    numBytes = sendto(socketfd,result,strlen(result),0,(struct sockaddr*)&clientAddr,sizeof(clientAddr));
    printf("The Server B finished sending the output to AWS\n");
  }
  close(socketfd);
}

void stopRunning(int sig_num) {
  keepRunning = FALSE;
  close(socketfd);
  printf("\n");
  exit(0);
}
