bplist00�_WebMainResource�	
_WebResourceData_WebResourceMIMEType_WebResourceTextEncodingName^WebResourceURL_WebResourceFrameNameOa<html><head></head><body><pre style="word-wrap: break-word; white-space: pre-wrap;">#include &lt;stdio.h&gt;
#include &lt;stdlib.h&gt;
#include &lt;string.h&gt;
#include &lt;unistd.h&gt;
#include &lt;sys/types.h&gt; 
#include &lt;sys/socket.h&gt;
#include &lt;netinet/in.h&gt;

#define PORT 4000

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, n;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
        printf("ERROR opening socket");
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&amp;(serv_addr.sin_zero), 8);     
    
	if (bind(sockfd, (struct sockaddr *) &amp;serv_addr, sizeof(serv_addr)) &lt; 0) 
		printf("ERROR on binding");
	
	listen(sockfd, 5);
	
	clilen = sizeof(struct sockaddr_in);
	if ((newsockfd = accept(sockfd, (struct sockaddr *) &amp;cli_addr, &amp;clilen)) == -1) 
		printf("ERROR on accept");
	
	bzero(buffer, 256);
	
	/* read from the socket */
	n = read(newsockfd, buffer, 256);
	if (n &lt; 0) 
		printf("ERROR reading from socket");
	printf("Here is the message: %s\n", buffer);
	
	/* write in the socket */ 
	n = write(newsockfd,"I got your message", 18);
	if (n &lt; 0) 
		printf("ERROR writing to socket");

	close(newsockfd);
	close(sockfd);
	return 0; 
}</pre></body></html>Ztext/plainUUTF-8_Thttps://moodle.inf.ufrgs.br/pluginfile.php/66108/mod_resource/content/2/server_tcp.cP    ( : P n } ��
a                           b