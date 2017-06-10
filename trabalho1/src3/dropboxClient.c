#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/inotify.h>

#define BUFFER_SIZE 1024
#define USERNAME_SIZE 100
#define SERVER_PORT 3002

#define CMD_UPLOAD 1
#define CMD_EXIT 2
#define CMD_ERROR -1
#define CMD_DOWNLOAD 3
#define CMD_SYNC 4

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN ( 1024 * ( EVENT_SIZE + 16 ) )

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

    if (strcmp(firstString, "download") == 0)
    {
        command = CMD_DOWNLOAD;
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
	// Declara e Inicializa
	char message[BUFFER_SIZE];

	FILE* filefd;
	int data_read = 0;
	int remain_data = 0;

	char* copy;
	char* command;
	char* filepath;
	char* filename;

	char filesize[50];
    
	// Pega dados do comando
	copy = strdup(buffer);
	command = strdup(strtok(copy, " "));
	filepath =  strdup(strtok(NULL, " "));
	
	// Tenta abrir arquivo
	filefd = fopen (filepath, "rb");
	if (filefd == NULL) {
		printf("Nao pode abrir arquivo\n");
		printf("Error is: %s (errno=%d)\n", strerror(errno), errno);
		return -1;
    }

    // Pega filename
    filename = strdup(basename(filepath));

    // Pega filesize
    fseek(filefd, 0L, SEEK_END);
    sprintf(filesize, "%d", ftell(filefd));
	rewind(filefd);
    
    // Monta comando
    memset(message, 0, BUFFER_SIZE);
	strcpy(message, "upload");
	strcat(message," ");
	strcat(message, filename);
	strcat(message," ");
	strcat(message, filesize);
    printf("MONTA COMANDO: COMMAND: %s\n", command);
    printf("MONTA COMANDO: FILENAME: %s\n", filename);
    printf("MONTA COMANDO: FILESIZE: %s\n", filesize);

    // Envia comando
    printf("ENVIA COMANDO: %s\n", message);
	send(sockfd, message, BUFFER_SIZE, NULL);

	// Envia arquivo
	remain_data = atoi(filesize);
	while (remain_data > 0 && (data_read = fread(&message, 1, BUFFER_SIZE, filefd)) > 0)
	{
		int data_sent = send(sockfd, message, data_read, NULL);
		remain_data -= data_sent;
	}

    // Limpa a sujeira
    fclose(filefd);
	free(copy);
}

int process_download(const int sockfd, const char* buffer)
{
	// Declara e Inicializa
	char message[BUFFER_SIZE];

    FILE* filefd;
    int data_read = 0;
    int remain_data = 0;

	char* copy;
	char* command;
	char* filename;
	char* filesize;

	char* copy2;
	char* command2;
	char* filename2;

	char* filepath;

	// Pega dados do comando do usuario
	copy = strdup(buffer);
	command = strdup(strtok(copy, " "));
	filename =  strdup(strtok(NULL, " "));

	// Monta comando
	memset(message, 0, BUFFER_SIZE);
	strcpy(message, "download");
	strcat(message," ");
	strcat(message, filename);

	// Envia comando
	send(sockfd, message, BUFFER_SIZE, NULL);

	// Espera resposta
	memset(message, 0, BUFFER_SIZE);
    recv(sockfd, message, BUFFER_SIZE, 0);

    // Trata resposta
    copy2 = strdup(message);
    command2 = strdup(strtok(copy2, " "));
    filename2 =  strdup(strtok(NULL, " "));
    filesize =  strdup(strtok(NULL, " "));
    remain_data = atoi(filesize);
	
    // Verifica erro
    if (strcmp(command, "ERROR") == 0)
    {
        printf("ERROR!\n");
        printf("%s\n", message);
        return -1;
    }

    // Monta filepath
    filepath = malloc(BUFFER_SIZE);
    memset(filepath, 0, BUFFER_SIZE);
    strcat(filepath, "./");
    strcat(filepath, filename);
    printf("FILEPATH: %s\n", filepath);

    // Cria arquivo
    filefd = fopen (filepath, "wb");
    if (filefd == NULL) {
    	printf("ERROR!\n");
		printf("Error: %d [%s]\n", errno, strerror(errno));
		return -1;
    }

    // Recebe arquivo
    memset(message, 0, BUFFER_SIZE);
    while (remain_data > 0 && (data_read = recv(sockfd, message, BUFFER_SIZE, NULL)) > 0)
	{
		fwrite(&message, 1, data_read, filefd);
		remain_data -= data_read;
		printf("RECV(): [%d/%d - %d]\n", remain_data, atoi(filesize), data_read);
		memset(message, 0, BUFFER_SIZE);
	}
	printf("Transfer finalizada\n");

    // Limpa a sujeira
    fclose(filefd);
    free(filepath);
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

int process_sync_server()
{
	// deleta tudo no server

	// envia tudo para server
}

int process_sync_client()
{
	// deleta tudo no client

	// recebe tudo do server
}



int create_sync_inotify_home_folder()
{
	int length;
	int i = 0;
	int fd;
	int wd;
	char buffer[EVENT_BUF_LEN];

	fd = inotify_init();
	if ( fd < 0 ) {
    	printf("inotify_init error: %d [%s]\n", errno, strerror(errno));
  	}

  	wd = inotify_add_watch(fd, "/home/mauricio/mauricioa", IN_CREATE | IN_DELETE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO );
  	if ( wd < 0 ) {
    	printf("inotify_add_watch error: %d [%s]\n", errno, strerror(errno));
  	}  

  	//blocking
  	length = read( fd, buffer, EVENT_BUF_LEN ); 
  	if ( length < 0 ) {
    	printf("read error: %d [%s]\n", errno, strerror(errno));
  	}  

	for (; i < length;) 
	{   
		struct inotify_event* event = (struct inotify_event*) &buffer[i];   

		if (event->len)
		{
			if(!(event->mask & IN_ISDIR))
			{
				if(event->name[0] == '.')
				{
					printf("*** %s ignored\n",event->name);
					i += EVENT_SIZE + event->len;
					continue;
				}
			}

			if ( event->mask & IN_CREATE ) 
			{
				if ( event->mask & IN_ISDIR ) 
				{
					printf( "*** IN_CREATE DIR: %s\n", event->name );
				}
				else 
				{
					printf( "*** IN_CREATE FILE: %s\n", event->name );
				}
			}
			else if ( event->mask & IN_DELETE ) 
			{
				if ( event->mask & IN_ISDIR ) 
				{
					printf( "*** IN_DELETE FILE: %s\n", event->name );
				}
				else 
				{
					printf( "*** IN_DELETE DIR: %s\n", event->name );
				}
			}
			else if ( event->mask & IN_MODIFY ) 
			{
				if ( event->mask & IN_ISDIR ) 
				{
					printf( "*** IN_MODIFY DIR: %s\n", event->name );
				}
				else 
				{
					printf( "*** IN_MODIFY FILE: %s\n", event->name );
				}
			}
			else if ( event->mask & IN_CLOSE_WRITE ) 
			{
				if ( event->mask & IN_ISDIR ) 
				{
					printf( "*** IN_CLOSE_WRITE DIR: %s\n", event->name );
				}
				else 
				{
					printf( "*** IN_CLOSE_WRITE FILE: %s\n", event->name );
				}
			}
			else if ( event->mask & IN_MOVED_TO ) 
			{
				if ( event->mask & IN_ISDIR ) 
				{
					printf( "*** IN_MOVED_TO DIR: %s\n", event->name );
				}
				else 
				{
					printf( "*** IN_MOVED_TO FILE: %s\n", event->name );
				}
			}
			else if ( event->mask & IN_MOVED_FROM ) 
			{
				if ( event->mask & IN_ISDIR ) 
				{
					printf( "*** IN_MOVED_FROM DIR: %s\n", event->name );
				}
				else 
				{
					printf( "*** IN_MOVED_FROM FILE: %s\n", event->name );
				}
			}
		}

		i += EVENT_SIZE + event->len;
	}

  	inotify_rm_watch( fd, wd );

  	close( fd );
}

int main()
{
	while(1)
	{
		create_sync_inotify_home_folder();
		printf("\n - \n");
	}
	

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

            		if(process_download(sockfd, buffer) < 0)
            		{
            			printf("Download failed.\n");
            			return -1;
            		} else {
            			printf("Download done.\n");
            		}

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
