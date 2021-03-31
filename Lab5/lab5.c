#include <stdio.h>  
#include <stdlib.h> 
#include <fcntl.h>  
#include <signal.h> 
#include <unistd.h> 
#include <string.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <sys/wait.h>


#define BUFSIZE 1024

volatile int stop = 0;
void intHandler(int signum);
void chldHandler(int signum);

int main(int argc, char **argv)
{
  int conn_fd, sockfd, yes =1;
  pid_t child_pid;
  struct sockaddr_in cln_addr, info;
  socklen_t sLen = sizeof(cln_addr);

  memset(&info, 0, sizeof(info)); 
  info.sin_family = AF_INET;
  info.sin_addr.s_addr = INADDR_ANY;

  if (argc != 2)
  {
    printf("Please usage: %s <port>\n", argv[0]);
    exit(-1);
  }

  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("Error create socket\n");
    exit(-1);
  }
  info.sin_port = htons((unsigned short)atoi(argv[1]));
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  bind(sockfd, (struct sockaddr *)&info, sizeof(info));
  listen(sockfd, 10);

  signal(SIGINT, intHandler);

  while (!stop)
  {
    conn_fd = accept(sockfd, (struct sockaddr *)&cln_addr, &sLen);
    if (conn_fd == -1)
    {
      perror("Error: accept()");
      continue;
    }

    child_pid = fork();
    if (child_pid >= 0)
    {
      if (child_pid == 0)
      {
        dup2(conn_fd, STDOUT_FILENO);
        close(conn_fd);
        execlp("sl", "sl", "-l", NULL);
        exit(-1);
      }
      else
      {
        printf("Train ID: %d\n", (int)child_pid);
      }
    }
  }
  close(sockfd);
  return 0;
}

void intHandler(int signum)
{
  stop = 1;
}