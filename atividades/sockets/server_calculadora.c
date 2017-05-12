#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PORT 8003

int main(int argc, char *argv[])
{
  char operador[1], op1[4], op2[4];
  float operando1;
  float operando2;
  float resultado;
  int s, nBytes;
  unsigned char buffer[256];
  struct sockaddr_in serveraddr, clientaddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size, client_addr_size;
  int i;

  s = socket(AF_INET, SOCK_DGRAM, 0);
  printf("socket: %d\n", s);

  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(PORT);
  serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serveraddr.sin_zero, '\0', sizeof serveraddr.sin_zero);

  bind(s, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
  printf("bind: %d\n", s);


  addr_size = sizeof serverStorage;

  while(1){
    printf("aguardando...\n");
    nBytes = recvfrom(s, buffer, 256, 0, (struct sockaddr *) &serverStorage, &addr_size);

    memcpy(operador, buffer, 1);

    memcpy(op1, (buffer + 10), sizeof(float));

    memcpy(op2, (buffer + 20), sizeof(float));

    operando1 = atoi(op1);
    operando2 = atoi(op2);

    switch (operador[0])
    {
        case '+':
            resultado = operando1 + operando2;
            break;
        case '-':
            resultado = operando1 - operando2;
            break;
        case '*':
            resultado = operando1 * operando2;
            break;
        case '/':
            resultado = operando1 / operando2;
            break;
        default:
            printf("operador nao suportado, use + - * /\n");
        }

    printf("op: %c\n", operador[0]);
    printf("n1: %f\n", operando1);
    printf("n2: %f\n", operando2);

    printf("resultado: %f\n", resultado);

    memcpy(buffer, &resultado, sizeof(float));

    printf("devolvendo: %s\n", buffer);

    sendto(s, buffer, 256, 0, (struct sockaddr *) &serverStorage, addr_size);
  }

  close(s);
  return 0;
}