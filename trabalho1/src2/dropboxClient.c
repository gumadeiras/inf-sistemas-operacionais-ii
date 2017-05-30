#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define USERNAME_SIZE 100
#define SERVER_PORT 3002

#define CMD_UPLOAD 1
#define CMD_EXIT 2
#define CMD_ERROR -1
#define CMD_DOWNLOAD 3
#define CMD_SYNC 4

char toLowercase(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? (ch + 32) : (ch);
}

void turnStringLowercase(char* string)
{
    for(int index=0; string[index]; index++)
    {
        string[index] = toLowercase(string[index]);
    }
}

int get_command_from_buffer(const char* string)
{
	char* copy = strdup(string);
    char* firstString;
    int command = CMD_ERROR;

    firstString = strtok(copy, " ");

    turnStringLowercase(firstString);

    if (strcmp(firstString, "upload") == 0)
    {
        command = CMD_UPLOAD;
    }

    if (strcmp(firstString, "exit") == 0)
    {
        command = CMD_EXIT;
    }

    free(copy);

    return command;
}

int process_upload(const int sockfd, const char* buffer)
{
	// file vars
	char* copy;
	char* command;
	char* path;
	char* filename;
	char filesize[50];
	int ret_r;
	FILE *file_s;
    char fbuffer[BUFFER_SIZE];
    int remain_data = 0;
    struct stat file_stat;

	copy = strdup(buffer);
	command = strtok(copy, " ");
	path =  strtok(NULL, " ");

	//path = malloc(200);
	//memset(path, 0 ,200);
	//strcpy(path, "/home/mauricio/Desktop/dog.jpg");

	filename = basename(path);
	memset(fbuffer, 0, BUFFER_SIZE);

	file_s = fopen (path, "r");
	if (file_s == NULL) {
		printf("Nao pode abrir arquivo\n");
		printf("Error is: %s (errno=%d)\n", strerror(errno), errno);
		return -1;
    }

	/*
    if (fstat(file_s, &file_stat) < 0)
    {
    	printf("Nao pode ler stat arquivo\n");
		printf("Error is: %s (errno=%d)\n", strerror( errno ), errno );
		break;
    }
    remain_data = file_stat.st_size;
    */

    fseek(file_s, 0L, SEEK_END);
	remain_data = ftell(file_s);
	rewind(file_s);

    sprintf(filesize, "%d", remain_data);

    memset(buffer, 0, BUFFER_SIZE);
	strcpy(buffer, "upload");
	strcat(buffer," ");
	strcat(buffer, filename);
	strcat(buffer," ");
	strcat(buffer, filesize);

	send(sockfd, buffer, BUFFER_SIZE, NULL);

	while (remain_data > 0 && (ret_r = fread(&fbuffer, 1, BUFFER_SIZE, file_s)) > 0)
	{
		int sx = send(sockfd, fbuffer, ret_r, NULL);
		remain_data -= sx;
		printf("File: [%d/%d - %d]\n", remain_data, atoi(filesize), sx);
	}

    fclose(file_s);
    
	free(copy);
}

int fill_buffer_with_command(char* buffer, int size)
{
    printf("\nDigite o comando:\n> ");

    memset(buffer, 0, BUFFER_SIZE);

    char* gets_r = fgets(buffer, BUFFER_SIZE, stdin);

    if(gets_r == NULL)
    {
    	return -1;
    }

    if(strlen(buffer) < 2)
    {
    	return -1;
    }

    if (strlen(buffer) > 1)
    {
    	buffer[strlen(buffer)-1] = '\0';
    } 
    else
    {
    	buffer[0] = '\0';
    }

    printf("Your command: [%s]\n", buffer);
}

int main()
{
    // Declara
	int sockfd;
    struct sockaddr_in serveraddr;
    char buffer[BUFFER_SIZE];
    char username[USERNAME_SIZE];

    // Inicializa
    memset(buffer, 0, BUFFER_SIZE);
    memset(username, 0, USERNAME_SIZE);

    // Cria
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1)
    {
        printf("Erro ao criar socket\n");
        return 1;
    }
    printf("Socket criado\n");
     
    // Configura
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serveraddr.sin_port = htons(SERVER_PORT);
 
    // Conecta
    if (connect(sockfd , (struct sockaddr *) &serveraddr , sizeof(serveraddr)) < 0)
    {
        printf("Erro ao conectar ao servidor\n");
        return 1;
    }
    printf("Conectado\n");

	// Envia HI
    memset(buffer, 0, BUFFER_SIZE);
	strcpy(buffer, "HI");
	if(send(sockfd, buffer, BUFFER_SIZE, 0)  < 0)
    {
        printf("Hi sent failed\n");
        return 1;
    }
    printf("Hi sent\n");

    // Recebe HI
    memset(buffer, 0, BUFFER_SIZE);
    if( recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        printf("recv failed\n");
        return 1;
    }
    printf("Hi feedback received\n");

    // Envia username
    memset(username, 0, USERNAME_SIZE);
    printf("Username: ");
    scanf("%s", username);

    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "USERNAME");
    strcat(buffer, " ");
    strcat(buffer, username);
    strcat(buffer, "\0");
    if(send(sockfd, buffer, BUFFER_SIZE, 0)  < 0)
    {
        puts("Send failed");
        return 1;
    }
    printf("Username enviado\n");

    // Recebe username
    memset(buffer, 0, BUFFER_SIZE);
    if( recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        printf("Recv failed\n");
        return 1;
    }
    printf("Username recebido\n");

    // Start the loop
    while(1)
    {
    	if(fill_buffer_with_command(buffer, BUFFER_SIZE) < 0)
    	{
    		continue;
    	}

       	switch(get_command_from_buffer(buffer))
        {
        	    case CMD_EXIT:
        	    	close(sockfd);
            		return 0;
            	break;

            	case CMD_UPLOAD:
            		if(process_upload(sockfd, buffer) < 0)
            		{
            			printf("Upload failed.\n");
            			return -1;
            		} else {
            			printf("Upload done.\n");
            		}

            	break;

            	case CMD_DOWNLOAD:
            		// todo: baixa o arquivo arg1 do servidor e salva no diretorio do path arg2

            	break;

            	case CMD_SYNC:
                    // todo: 

            	break;

            	default:

            		// Envia
		        	if(send(sockfd, buffer, BUFFER_SIZE, 0)  < 0)
			        {
			            printf("Send command failed\n");
			            return 1;
			        }
			        printf("Command [%s] enviado.\n", buffer);

			        // Recebe
			        memset(buffer, 0, BUFFER_SIZE);
			        if( recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
			        {
			            printf("Recv failed\n");
			            return 1;
			        }
			        printf("Server respondeu: [%s]\n", buffer);
                    
            	break;
        }

    }
     
    close(sockfd);

	return 0;
}
