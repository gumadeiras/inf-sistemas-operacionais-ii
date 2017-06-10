//*
//* Trabalho 1
//* Instituto de Informática - UFRGS
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxClient.c
//* implementação das funções do cliente
//*
//* chamada: ./dropboxClient user ip port
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

int sockfd, n; // socket file descriptor
struct sockaddr_in serv_addr; // server address information
struct hostent *server;

int main(int argc, char *argv[])
{
  char buffer[256];
  int PORT = atoi(argv[3]);


  // verifica que os parâmetros usuario, ip e porta foram passados
  if (argc < 4)
  {
    fprintf(stderr,"[client]: usage %s <username> <ip> <port>\n", argv[0]);
    exit(0);
  }

  printf("[client]: atempting to connect: \"%s@%s:%s\"\n", argv[1], argv[2], argv[3]);

  // chama função que cuida da conexão com o servidor
  connect_server(argv[2], PORT);

  printf("[client]: connected to server: \"%s@%s:%d\"\n", argv[1], inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

  // chama função para verificar qual usuário está se conectando ao servidor
  verify_user_client(argv[1]);

  while(1)
  {
    printf("[client-comando]:~> ");
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
  }

  close(sockfd);
  return 0;
}

//* Conecta o cliente com o servidor.
//* host – endereço do servidor
//* port – porta aguardando conexão
int connect_server(char *host, int port)
{

  // função gethostbyname procura algum servidor no ip fornecido
  if ((server = gethostbyname(host)) == NULL) {  // get the server info
        fprintf(stderr,"[client]: ERROR, no such host\n");
        perror("[client]: gethostbyname");
        exit(1);
  }

  // socket() abre o socket para se conectar ao servidor
  if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
  {
    printf("[client]: ERROR opening socket\n");
    perror("[client]:socket");
    exit(1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
  bzero(&(serv_addr.sin_zero), 8);

  // função connect() realiza a conexão em si com o servidor
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("[client]: ERROR connecting\"%s:%d\"\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    perror("[client]: connect");
    exit(1);
  }
}

//* Sincroniza o diretório “sync_dir_<nomeusuário>” com o servidor.
void sync_client()
{

}

//* Envia um arquivo file para o servidor. Deverá ser executada quando for realizar upload de um arquivo. file – path/filename.ext do arquivo a ser enviado
void send_file(char *file)
{


/* 
fopen function: 

FILE *fopen(const char *filename, const char *mode)

filename − This is the C string containing the name of the file to be opened.
mode − This is the C string containing a file access mode.

"r"	Opens a file for reading. The file must exist.
"w"	Creates an empty file for writing. If a file with the same name already exists, its content is erased and the file is considered as a new empty file.
"a"	Appends to a file. Writing operations, append data at the end of the file. The file is created if it does not exist.
"r+"	Opens a file to update both reading and writing. The file must exist.
"w+"	Creates an empty file for both reading and writing.
"a+"	Opens a file for reading and appending.
*/

/* Opening the file the user wants to send */
  FILE *fp = fopen(*file, r);
  if(fp==null)
  {
    printf("Opening file error");
    return 1;
  }

  /* 
  Is this correct? We need to know the size of the file opened to read it properly. 
  I dont know if the right reference is fp, *fp or something else. 
  */
  size_t bytesToWrite = sizeof(fp);
  size_t bytesWritten = 0;

  while (bytesWritten != bytesToWrite)
  {
      size_t writtenThisTime;

      do
      {
        writtenThisTime = write(sockfd, buffer + bytesWritten, (bytesToWrite - bytesWritten));
      }
      while(writtenThisTime == -1) && (errno == EINTR));

      if(writtenThisTime == -1)
      {
          /* Real error. Do something appropriate */
          return;
      }
      bytesWritten += writtenThisTime;
  }

/*
 Please keep in mind that this needs review from every possible angle.
 IMPORTANT: THIS WAS NOT TESTED. I do not have any Linux machine/virtual machine avaible at the moment.
*/



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

int verify_user_client(char *username)
{
  n = write(sockfd, username, 18);
    if (n < 0)
    {
      printf("[client]: ERROR writing username to socket\n");
      return -1;
    }

  return 0;
}
