//*
//* Trabalho 1
//* Instituto de Informática - UFRGS
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxClient.c
//* implementação das funções do cliente
//*
//* chamada: ./dropboxClient fulano endereço porta
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

#include "./dropboxServer.h"

int main(int argc, char *argv[])
{
  int sockfd, n; // socket file descriptor
  struct sockaddr_in serv_addr; // server address information
  struct hostent *server;
  char buffer[256];
  int PORT = atoi(argv[3]);

  if (argc < 4)
  {
    fprintf(stderr,"[client]: usage %s <username> <ip> <port>\n", argv[0]);
    exit(0);
  }

  printf("[client]: atempting to connect: \"%s@%s:%s\"\n", argv[1], argv[2], argv[3]);
  // server = gethostbyname(argv[2]);
  // if (server == NULL)
  // {
  //   fprintf(stderr,"[client]: ERROR, no such host\n");
  //   exit(0);
  // }

  if ((server = gethostbyname(argv[2])) == NULL) {  // get the server info
        fprintf(stderr,"[client]: ERROR, no such host\n");
        perror("[client]: gethostbyname");
        exit(1);
  }

  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
  {
    printf("[client]: ERROR opening socket\n");
    perror("[client]:socket");
    exit(1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
  bzero(&(serv_addr.sin_zero), 8);

  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("[client]: ERROR connecting\"%s:%d\"\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    perror("[client]: connect");
    exit(1);
  }

  printf("[client]: message to send: ");
  bzero(buffer, 256);
  fgets(buffer, 256, stdin);

  /* write in the socket */
  if ((n = write(sockfd, buffer, strlen(buffer))) == -1)
  {
    printf("[client]: ERROR writing to socket\n");
    perror("[client]: write");
    exit(1);
  }

  bzero(buffer,256);

  /* read from the socket */
  if ((n = read(sockfd, buffer, 256)) == -1)
  {
    printf("[client]: ERROR reading from socket\n");
    perror("[client]: read");
    exit(1);
  }

  printf("%s\n", buffer);

  close(sockfd);
  return 0;
}

//* Conecta o cliente com o servidor.
//* host – endereço do servidor
//* port – porta aguardando conexão
int connect_server(char *host, int port)
{

}

//* Sincroniza o diretório “sync_dir_<nomeusuário>” com o servidor.
void sync_client()
{

}

//* Envia um arquivo file para o servidor. Deverá ser executada quando for realizar upload de um arquivo. file – path/filename.ext do arquivo a ser enviado
void send_file(char *file)
{

}

//* Obtém um arquivo file do servidor.
//* Deverá ser executada quando for realizar download de um arquivo.
//* file –filename.ext
void get_file(char *file)
{

}

//* Fecha a conexão com o servidor.
void close_connection()
{

}