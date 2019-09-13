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

#define UDP_PORT 21621
#define MAXBUFLEN 1024
#define BUFSIZE 1024
#define TRUE 1
#define FALSE 0

int socketfd;
int keepRunning = TRUE;

void stopRunning(int sig_num);
void updateDataBase(char buf[], int *linkID);
int getFromDataBase(char buf[], char result[], int linkID);

int main(void) {
  int numBytes;
  int clientAddrLen;
  int receivedValidData = FALSE;
  int linkID = 1;

  FILE *fp;



  struct sockaddr_in serverAddr;
  struct sockaddr_in clientAddr;

  char buf[BUFSIZE] = "";
  char result[BUFSIZE] = "";
  char resp[BUFSIZE]= "";
  char command[30];
  char writecmd[] = "write";
  char computecmd[] = "compute";

  int computeLinkID;

  signal(SIGINT, stopRunning);
  // Create a UDP socket
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
  // Create database.txt
  fp = fopen("database.txt","w");
  fclose(fp);

  printf("The Server A is up and running using UDP on port <%i>.\n",UDP_PORT);

  // Server in continous loop
  /****************************************************************************
   * The while loop and the connection/binding code was inspired from the
   * Beej socket prgramming tutorial
   ****************************************************************************/
  while(keepRunning == TRUE) {
    memset(buf,0,sizeof buf);
    memset(resp,0,sizeof resp);
    memset(result,0,sizeof result);
    numBytes = recvfrom(socketfd, buf, BUFSIZE-1,0,(struct sockaddr*)&clientAddr,(socklen_t*)&clientAddrLen);
     if(numBytes < 0){
       perror("server: recvfrom");
     } else {
       receivedValidData = TRUE;
       buf[numBytes] = '\0';
     }
     if(receivedValidData == TRUE) {
       if(strstr(buf,writecmd) != NULL) {
         printf("The Server A received input for writing\n");
         updateDataBase(buf,&linkID);
         // TODO: Send a response to AWS
         strcpy(resp,"finished");
         numBytes = sendto(socketfd,resp,strlen(resp),0,(struct sockaddr*)&clientAddr,sizeof(clientAddr));
         printf("The Server A wrote link <%i> to database\n",linkID-1);
       }
       if(strstr(buf,computecmd) != NULL) {
         sscanf(buf,"%s %i",command,&computeLinkID);
         printf("The Server A received input <%i> for computing\n",computeLinkID);
         if(getFromDataBase(buf,result,linkID) == 1) {
           strcat(resp,"success ");
           strcat(resp,result);
           numBytes = sendto(socketfd,resp,strlen(resp),0,(struct sockaddr*)&clientAddr,sizeof(clientAddr));
           printf("The Server A finished sending the search result to AWS\n");
         } else {
           printf("Link ID not found\n");
           strcat(resp,"failure");
           numBytes = sendto(socketfd,resp,strlen(resp),0,(struct sockaddr*)&clientAddr,sizeof(clientAddr));
         }
       }
     }
  }
  close(socketfd);
}

void stopRunning(int sig_num) {
  close(socketfd);
  printf("\n");
  exit(0);
}

void updateDataBase(char buf[], int *linkID) {
  char command[BUFSIZE];
  double bw;
  double length;
  double velocity;
  double noisepower;

  FILE *fp;

  // Open file such that we always append to the end
  fp = fopen("database.txt","a");
  sscanf(buf,"%s %lf %lf %lf %lf",command,&bw,&length,&velocity,&noisepower);
  fprintf(fp,"%i %lf %lf %lf %lf\n",*linkID,bw,length,velocity,noisepower);
  *linkID = *linkID + 1;
  fclose(fp);

}

int getFromDataBase(char buf[],char result[],int linkID) {
  FILE *fp;
  int userLinkId;
  int storedLinkID;
  int len = BUFSIZE-1;
  char command[BUFSIZE];

  sscanf(buf,"%s %i",command,&userLinkId);
  if(userLinkId >= linkID) {
    return -1;
  }
  fp = fopen("database.txt","r");
  printf("The Server A recieved input <%i> for computing\n",userLinkId);
  for(int entry=0;entry<linkID;entry++) {
    getline(&result,(size_t*)&len,fp);
    sscanf(result,"%i",&storedLinkID);
    if(storedLinkID == userLinkId) {
      fclose(fp);
      return 1;
    }
  }
  fclose(fp);
  return -1;
}
