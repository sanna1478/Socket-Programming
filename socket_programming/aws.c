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

#define UDP_PORT 23621
#define TCP_PORT_CLI 24621
#define TCP_PORT_MON 25621

#define TRUE 1
#define FALSE 0

#define WRITE 1
#define COMPUTE 2
#define INVALID -1

#define BUFSIZE 1024

int keepRunning = TRUE;
int parentTCPSocketfd = -1;
int parentTCPMonitorSocketfd = -1;
int clientTCPChildSocketfd = -1;
int monitorTCPChildSocketfd = -1;

void stopRunning(int signum);
void connectUDPServer(struct sockaddr_in *awsAddr, char buf[],char recvFrom[],int port);

int main(void) {
  int numBytes;
  int cliLen;
  int monLen;
  int yes = 1;

  int linkid,size;
  double sigpower;

  int serverA_port = 21621;
  int serverB_port = 22621;

  char buf[BUFSIZE] = "";
  char recvFrom[BUFSIZE] = "";
  char result[BUFSIZE] = "";
  char command[30];

  //memset(buf,0,sizeof buf);
  //memset(recvFrom,0,sizeof recvFrom);
  char writecmd[] = "write";
  char computecmd[] = "compute";


  struct sockaddr_in aws_TCPCliServerAddr;
  struct sockaddr_in aws_TCPMonServerAddr;
  struct sockaddr_in clientAddr;
  struct sockaddr_in monitorAddr;

  struct sockaddr_in UDPClientAddr;


  signal(SIGINT, stopRunning);

  parentTCPMonitorSocketfd = socket(AF_INET, SOCK_STREAM, 0);
  if(parentTCPMonitorSocketfd < 0) {
    perror("aws: socket");
    exit(1);
  }
  parentTCPSocketfd = socket(AF_INET, SOCK_STREAM, 0);
  if(parentTCPSocketfd < 0) {
    perror("aws: socket");
    exit(1);
  }

  printf("The AWS is up and running\n");

  // initalise the TCP server connecting to client
  aws_TCPCliServerAddr.sin_family = AF_INET;
  aws_TCPCliServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  aws_TCPCliServerAddr.sin_port = htons(TCP_PORT_CLI);

  // initalise the TCP server connecting to monitor
  aws_TCPMonServerAddr.sin_family = AF_INET;
  aws_TCPMonServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  aws_TCPMonServerAddr.sin_port = htons(TCP_PORT_MON);

  // initalise the UDP client
  UDPClientAddr.sin_family = AF_INET;
  UDPClientAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  UDPClientAddr.sin_port = htons(UDP_PORT);

  if(setsockopt(parentTCPSocketfd,SOL_SOCKET,SO_REUSEADDR,&yes, sizeof yes) == -1) {
    perror("setsockopt");
    exit(1);
  }
  if(setsockopt(parentTCPMonitorSocketfd,SOL_SOCKET,SO_REUSEADDR,&yes, sizeof yes) == -1) {
    perror("setsockopt");
    exit(1);
  }


  // bind the TCP server connecting to client to the port
  bind(parentTCPSocketfd, (struct sockaddr*)&aws_TCPCliServerAddr,sizeof(aws_TCPCliServerAddr));
  bind(parentTCPMonitorSocketfd, (struct sockaddr*)&aws_TCPMonServerAddr,sizeof(aws_TCPMonServerAddr));

  // Listen for client TCP connection request
  if(listen(parentTCPSocketfd,0) < 0) {
    close(parentTCPSocketfd);
    perror("aws: listen");
    exit(1);
  }

  // Listen for monitor TCP connection request
  if(listen(parentTCPMonitorSocketfd,0) < 0) {
    close(parentTCPMonitorSocketfd);
    perror("aws: listen");
    exit(1);
  }

  cliLen = sizeof(clientAddr);
  monLen = sizeof(monitorAddr);
  /****************************************************************************
   * The while loop and the connection/binding code was inspired from the
   * Beej socket prgramming tutorial
   ****************************************************************************/
  while(keepRunning == TRUE) {
    clientTCPChildSocketfd = accept(parentTCPSocketfd, (struct sockaddr*)&clientAddr,(socklen_t*)&cliLen);

    if(monitorTCPChildSocketfd == -1){
      monitorTCPChildSocketfd = accept(parentTCPMonitorSocketfd, (struct sockaddr*)&monitorAddr,(socklen_t*)&monLen);
      shutdown(monitorTCPChildSocketfd,SHUT_RD);
    }

    if(keepRunning == FALSE) {
      close(clientTCPChildSocketfd);
      close(monitorTCPChildSocketfd);
      break;
    }
    if(clientTCPChildSocketfd < 0) {
      close(parentTCPSocketfd);
      if(monitorTCPChildSocketfd > 0) {
        close(monitorTCPChildSocketfd);
      }
      close(parentTCPMonitorSocketfd);
      perror("aws_client: accept");
      exit(1);
    }
    if(monitorTCPChildSocketfd < 0) {
      close(parentTCPMonitorSocketfd);
      if(clientTCPChildSocketfd > 0) {
        close(clientTCPChildSocketfd);
      }
      close(parentTCPSocketfd);
      perror("aws_client: accept");
      exit(1);
    }

    // Wait for client to get data
    numBytes = recv(clientTCPChildSocketfd,buf,BUFSIZE-1,0);
    if(numBytes < 0) {
      close(clientTCPChildSocketfd);
      close(parentTCPSocketfd);
      perror("aws_client: recv");
      exit(1);
    }
    buf[numBytes] = '\0';

    // Print message that AWS received data from client
    if(strstr(buf,writecmd) != NULL) {
      printf("The AWS received operation <write> from the client using TCP over port <%u>\n",ntohs(aws_TCPCliServerAddr.sin_port));
      /*
      TODO:
        1. Send message to monitor
        2. Send parsed data to Server A
        3. Send sucess message to monitor and client
      */
      printf("The AWS sent operation <write> and arguments to the monitor using TCP over port <%i>\n",TCP_PORT_MON);
      numBytes = send(monitorTCPChildSocketfd,buf,strlen(buf),0);
      if(numBytes < 0) {
        perror("aws monitor: send");
      } /*else {
        printf("expected bytes sent: %lu, actual bytes sent: %i\n",strlen(buf),numBytes);
      }*/

      printf("The AWS sent operation <write> to Backend-Server A using UDP over port <%i>\n",UDP_PORT);
      connectUDPServer(&UDPClientAddr,buf,recvFrom,serverA_port);
      printf("The AWS received operation <write> from Backend-Server A for writing using UDP over port <%i>\n",UDP_PORT);

      numBytes = send(clientTCPChildSocketfd,recvFrom,strlen(recvFrom),0);
      if(numBytes < 0) {
        perror("aws monitor: send");
      } /*else {
        printf("expected bytes sent: %lu, actual bytes sent: %i\n",strlen(recvFrom),numBytes);
      }*/
      printf("The AWS sent result to client for operation <write> using TCP over port <%i>\n",TCP_PORT_CLI);

      numBytes = send(monitorTCPChildSocketfd,recvFrom,strlen(recvFrom),0);
      if(numBytes < 0) {
        perror("aws monitor: send");
      } /*else {
        printf("expected bytes sent: %lu, actual bytes sent: %i\n",strlen(recvFrom),numBytes);
      }*/
      printf("The AWS sent write response to the monitor using TCP over port <%i>\n",TCP_PORT_MON);


    }
    if(strstr(buf,computecmd) != NULL) {
      char m[10];
      printf("The AWS received operation <compute> from the client using TCP over port <%u>\n",ntohs(aws_TCPCliServerAddr.sin_port));
      /*
      TODO:
        1. Send parsed data to Server A
        2. If LINK_ID exists send data to server B
        3. save the result of Server B in buf
        4. if LINK_ID deosn't exists update buf
      */
      printf("The AWS sent operation <compute> and arguments to the monitor using TCP over port <%i>\n",TCP_PORT_MON);
      numBytes = send(monitorTCPChildSocketfd,buf,strlen(buf),0);
      if(numBytes < 0) {
        perror("aws monitor: send");
      }
      printf("The AWS sent opertaion <compute> to Backend-Server A using UDP over port <%i>\n",UDP_PORT);
      connectUDPServer(&UDPClientAddr,buf,recvFrom,serverA_port);
      printf("The result of compute searching: %s\n",recvFrom);
      sscanf(recvFrom,"%s",m);
      if(strcmp(m,"success") == 0) {
        printf("The AWS received link information from Backend-Server A using UDP over port <%i>\n",UDP_PORT);
        sscanf(buf,"%s %i %i %lf",command,&linkid,&size,&sigpower);

        strcat(buf," ");
        strcat(buf,recvFrom);
        printf("The AWS sent link ID=<%i>, size=<%i>, power=<%.2lf>, and link information to Backend-Server B using UDP over port <%i>",linkid,size,sigpower,UDP_PORT);
        connectUDPServer(&UDPClientAddr,buf,recvFrom,serverB_port);
        printf("The AWS recieved outputs from Backend-Server B using UDP over port <%i>",UDP_PORT);
        strcpy(result,"success");
        strcat(result, " ");
        strcat(result,recvFrom);

        numBytes = send(clientTCPChildSocketfd,result,strlen(result),0);
        printf("The AWS sent result to client for operation <compute> using TCP over port <%i>\n",TCP_PORT_CLI);
        numBytes = send(monitorTCPChildSocketfd,recvFrom,strlen(result),0);
        printf("The AWS sent compute results to the monitor using TCP over port <%i>\n",TCP_PORT_MON);

      } else {
        printf("Link ID not found\n");
        numBytes = send(clientTCPChildSocketfd,recvFrom,strlen(recvFrom),0);
        printf("The AWS sent result to client for operation <compute> using TCP over port <%i>\n",TCP_PORT_CLI);
        numBytes = send(monitorTCPChildSocketfd,recvFrom,strlen(recvFrom),0);
        printf("The AWS sent compute results to the monitor using TCP over port <%i>\n",TCP_PORT_MON);

      }
    }
    // echo input to client
    /*numBytes = send(clientTCPChildSocketfd,buf,strlen(buf),0);
    if(numBytes < 0) {
      close(clientTCPChildSocketfd);
      close(parentTCPSocketfd);
      perror("aws_client: send");
      exit(1);
    }*/
    close(clientTCPChildSocketfd);
  }
  close(monitorTCPChildSocketfd);
  close(parentTCPSocketfd);
  close(parentTCPMonitorSocketfd);
}

void stopRunning(int signum) {
  keepRunning = FALSE;
  if(clientTCPChildSocketfd != -1) {
    close(clientTCPChildSocketfd);
  }
  if(monitorTCPChildSocketfd != -1) {
    close(monitorTCPChildSocketfd);
  }
  if(parentTCPSocketfd != -1) {
    close(parentTCPSocketfd);
  }
  if(parentTCPMonitorSocketfd != -1) {
    close(parentTCPMonitorSocketfd);
  }
  printf("\n");
  exit(0);
}

void connectUDPServer(struct sockaddr_in *awsAddr, char buf[], char recvFrom[], int port) {
  int socketfd;
  int serverLen;
  int numBytes;
  int yes = 1;
  struct sockaddr_in server;

  socketfd = socket(AF_INET,SOCK_DGRAM, 0);
  if(socketfd < 0) {
    perror("aws serverA: socket");
    exit(1);
  }

  if(setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,&yes, sizeof yes) == -1) {
    perror("setsockopt");
    exit(1);
  }

  // The awsAddr is already initalized
  if(bind(socketfd,(struct sockaddr*)awsAddr, sizeof(*awsAddr)) < 0) {
    perror("UDP client: bind");
    exit(1);
  }
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr("127.0.0.1");

  serverLen = sizeof(server);
  numBytes = sendto(socketfd,buf,strlen(buf),0,(struct sockaddr*)&server,serverLen);
  if(numBytes < 0) {
    perror("aws serverA: sendto");
    exit(1);
  }
  numBytes = recvfrom(socketfd,recvFrom,BUFSIZE-1,0,(struct sockaddr*)&server,(socklen_t*)&serverLen);
  recvFrom[numBytes] = '\0';
  close(socketfd);
}
