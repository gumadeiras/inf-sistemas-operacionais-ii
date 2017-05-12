#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define BUFLEN 256
#define PORT 8003

int main(int argc, char *argv[])
{
  struct sockaddr_in socket_sv, socket_client;
  int s, b, i, slen=sizeof(socket_client), bytes;
  unsigned char buf[BUFLEN];
  socklen_t addr_size;

  if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    printf("socket ruim\n");

  memset((char *) &socket_sv, 0, sizeof(socket_sv));

  socket_sv.sin_family = AF_INET;
  socket_sv.sin_port = htons(PORT);
  socket_sv.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_size = sizeof(socket_sv);


  while(1)
  {

  printf("digite a operacao:\n");
  scanf("%s", buf);

  printf("digite um numero:\n");
  scanf("%s", buf+10);

  printf("digite outro numero:\n");
  scanf("%s", buf+20);

  sendto(s, buf, BUFLEN, 0, (struct sockaddr *) &socket_sv, addr_size);
  printf("enviado: %s\n", buf);

  recvfrom(s, buf, BUFLEN, 0, NULL, NULL);

  printf("resultado: %s\n", buf);
  }
  return 0;
}
