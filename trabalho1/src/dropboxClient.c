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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "./dropboxServer.h"

#define PORT 4000

int main(int argc, char *argv[])
{
  int sockfd, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  char buffer[256];
  if (argc < 4)
  {
    fprintf(stderr,"usage %s username ip port\n", argv[0]);
    exit(0);
  }

  server = gethostbyname(argv[2]);
  if (server == NULL)
  {
    fprintf(stderr,"ERROR, no such host\n");
    exit(0);
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    printf("ERROR opening socket\n");

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
  bzero(&(serv_addr.sin_zero), 8);

  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    printf("ERROR connecting\n");

  printf("Enter the message: ");
  bzero(buffer, 256);
  fgets(buffer, 256, stdin);

  /* write in the socket */
  n = write(sockfd, buffer, strlen(buffer));
  if (n < 0)
    printf("ERROR writing to socket\n");

  bzero(buffer,256);

  /* read from the socket */
  n = read(sockfd, buffer, 256);
  if (n < 0)
    printf("ERROR reading from socket\n");

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