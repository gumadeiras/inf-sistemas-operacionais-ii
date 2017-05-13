//*
//* Trabalho 1
//* Instituto de Informática - UFRGS
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxServer.c
//* implementação das funções do servidor
//*
//*

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "dropboxServer.h"

#define PORT 4000
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



int main(int argc, char *argv[])
{
  int sockfd, newsockfd, n;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  char buffer[256];

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    printf("ERROR opening socket");

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(serv_addr.sin_zero), 8);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    printf("ERROR on binding");

  listen(sockfd, 5);

  clilen = sizeof(struct sockaddr_in);
  if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
    printf("ERROR on accept");

  bzero(buffer, 256);

  /* read from the socket */
  n = read(newsockfd, buffer, 256);
  if (n < 0)
    printf("ERROR reading from socket");
  printf("Here is the message: %s\n", buffer);

  /* write in the socket */
  n = write(newsockfd,"I got your message", 18);
  if (n < 0)
    printf("ERROR writing to socket");

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