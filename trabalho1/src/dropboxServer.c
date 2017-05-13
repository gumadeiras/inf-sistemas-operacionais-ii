//*
//* Trabalho 1
//* Instituto de Informática - UFRGS
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxServer.c
//* implementação das funções do servidor
//*
//* chamada: ./dropboxServer ip port
//*


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "dropboxServer.h"

#define MAXNAME 256

struct client
{
  int devices[2]; // associado aos dispositivos do usuário userid[MAXNAME]
  char userid[MAXNAME]; // id do usuário no servidor, que deverá ser único. Informado pela linha de comando.
  struct file_info; // metadados de cada arquivo que o cliente possui no servidor.
  int logged_in; // cliente está logado ou não.
};

struct file_info
{
  char name[MAXNAME]; // refere-se ao nome do arquivo. extension[MAXNAME]
  char extension[MAXNAME]; // refere-se ao tipo de extensão do arquivo.
  char last_modified[MAXNAME]; // refere-se a data da última mofidicação no arquivo.
  int size; // indica o tamanho do arquivo, em bytes.
};

// server information
int sockfd, PORT;
struct sockaddr_in serv_addr;

int main(int argc, char *argv[])
{
  int newsockfd; // new socket on accept() return; pthreads para gerenciar várias conexões?
  int n;
  struct sockaddr_in cli_addr;
  socklen_t clilen;
  char buffer[256];

  if (argc != 3)
  {
    fprintf(stderr,"[server]: usage %s <ip> <port>\n", argv[0]);
    exit(0);
  }

  printf("[server]: atempting to create socket: \"%s:%s\"\n", argv[1], argv[2]);

  server_init(argv[1], argv[2]);

  printf("[server]: server is up and running!\n");
  printf("[server]: listening on %s:%d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

  while(1)
  {

    clilen = sizeof(struct sockaddr_in);
    if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
    {
      printf("[server]: ERROR on accept\n");
      perror("[server]: accept");
      exit(1);
    }


    printf("[server]: incoming connection from %s\n", inet_ntoa(cli_addr.sin_addr));

    bzero(buffer, 256);

    // int getpeername(int sockfd, struct sockaddr *addr, int *addrlen);
    // int gethostname(char *hostname, size_t size);

    // https://www.gta.ufrj.br/ensino/eel878/sockets/clientserver.html
    // if (!fork()) { // this is the child process
    //           close(sockfd); // child doesn't need the listener
    //           if (send(new_fd, "Hello, world!\n", 14, 0) == -1)
    //               perror("send");
    //           close(new_fd);
    //           exit(0);
    //       }
    // close(new_fd); // parent doesn't need this


    /* read from the socket */
    n = read(newsockfd, buffer, 256);
    if (n < 0)
      printf("[server]: ERROR reading from socket\n");
    if (n == 0)
      printf("[server]: ERROR reading from socket: connection closed by client\n");

    printf("[server]: message received: %s\n", buffer);

    /* write in the socket */
    n = write(newsockfd,"[server]: I got your message\n", 18);
    if (n < 0)
      printf("[server]: ERROR writing to socket\n");
  }

  close(newsockfd);
  close(sockfd);
  return 0;
}

//* Sincroniza o servidor com o diretório “sync_dir_<nomeusuário>” com o cliente.
void sync_server()
{

}

//* Recebe um arquivo file do cliente.
//* Deverá ser executada quando for realizar upload de um arquivo. file – path/filename.ext do arquivo a ser recebido
void receive_file(char *file)
{

}

//* Envia o arquivo file para o usuário.
//* Deverá ser executada quando for realizar download de um arquivo. file – filename.ext
void send_file(char *file)
{

}

int server_init(char *s_ip, char *s_port)
{
  PORT = atoi(s_port);

  #ifdef DEBUG
  PRINTF("%s %s \n", s_ip, s_port);
  #endif

  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
  {
    printf("[server]: ERROR opening socket\n");
    perror("[server]: socket");
    exit(1);
  }

  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
  {
    perror("[server]: setsockopt");
    exit(1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = inet_addr(s_ip);
  bzero(&(serv_addr.sin_zero), 8);

  #ifdef DEBUG
  PRINTF("%d\n", ntohs(serv_addr.sin_port));
  #endif

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("[server]: ERROR on binding \"%s:%d\"\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    perror("[server]: bind");
    exit(1);
  }

  if (listen(sockfd, 5) == -1)
  {
    perror("[server]: listen");
    exit(1);
  }
}