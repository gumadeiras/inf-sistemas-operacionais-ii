//*
//* Trabalho 1
//* Instituto de Informática - UFRGS
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxClient.c
//* implementação das funções do cliente
//*

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <linux/inotify.h>
#include <pwd.h>

#define BUFFER_SIZE 1024
#define USERNAME_SIZE 100
#define SERVER_PORT 3002
#define IP_SIZE 15

#define CMD_ERROR -1
#define CMD_UPLOAD 1
#define CMD_EXIT 2
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
	int index;
    for(index=0; string[index]; index++)
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

// void send_file(char *file);
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

    // Pega dados do comando que estao separados por um espaco
    copy = strdup(buffer);
    command = strdup(strtok(copy, " "));
    filepath =  strdup(strtok(NULL, " "));

    // Tenta abrir arquivo
    filefd = fopen (filepath, "rb");
    if (filefd == NULL) {
        printf("[client] Nao pode abrir arquivo\n");
        printf("[client] Error is: %s (errno=%d)\n", strerror(errno), errno);
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
    // printf("[client] MONTA COMANDO: COMMAND: %s\n", command);
    // printf("[client] MONTA COMANDO: FILENAME: %s\n", filename);
    // printf("[client] MONTA COMANDO: FILESIZE: %s\n", filesize);

    // Envia comando
    // printf("[client] ENVIA COMANDO: %s\n", message);
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

// void receive_file(char *file);
int process_download(const int sockfd, const char* buffer, char* username)
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
    char* syncpath;

    // Pega dados do comando do usuario separados por espaco
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
        printf("[client] ERROR!\n");
        printf("[client] %s\n", message);
        return -1;
    }

    // Cria a pasta sync na home do usuario se ela nao existe

    // home
    struct passwd* pw = getpwuid(getuid());
    char* homedir = strdup(pw->pw_dir);
    syncpath = malloc(BUFFER_SIZE);
    memset(syncpath, 0, BUFFER_SIZE);
    strcpy(syncpath, homedir);
    strcat(syncpath, "/sync_dir_");
    strcat(syncpath, username);
    strcat(syncpath, "/");
    if(mkdir(syncpath, 0700) < 0)
    {
        printf("[client] mkdir error (pasta ja existe) (errstr=%s) (errno=%d)\n", strerror(errno), errno);
    }
    printf("SYNCPATH: %s\n", syncpath);

    // Monta filepath
    printf("%s\n", username);
    filepath = malloc(BUFFER_SIZE);
    memset(filepath, 0, BUFFER_SIZE);
    strcpy(filepath, syncpath);
    strcat(filepath, filename);
    printf("FILEPATH: %s\n", filepath);

    // Cria arquivo
    filefd = fopen (filepath, "wb");
    if (filefd == NULL) {
        printf("[client] fopen error (errstr=%s) (errno=%d)\n", strerror(errno), errno);
        return -1;
    }

    // Recebe arquivo
    memset(message, 0, BUFFER_SIZE);
    while (remain_data > 0 && (data_read = recv(sockfd, message, BUFFER_SIZE, NULL)) > 0)
    {
        fwrite(&message, 1, data_read, filefd);
        remain_data -= data_read;
        printf("[client] RECV(): [%d/%d - %d]\n", remain_data, atoi(filesize), data_read);
        memset(message, 0, BUFFER_SIZE);
    }
    printf("[client] transfer completed\n");

    // Limpa a sujeira
    fclose(filefd);
    free(filepath);
}

int deleteAllFiles(char* folderpath)
{
    DIR* folder = opendir(folderpath);
    struct dirent* file;
    char filepath[256];

    while ((file = readdir(folder)) != NULL)
    {
        sprintf(filepath, "%s/%s", folderpath, file->d_name);
        remove(filepath);
    }

    closedir(folder);
}

int fill_buffer_with_command(char* buffer, int size)
{
    printf("\n\n\n-+-~-+--+-~-+--+-~-+--+-~-+--+-~-+--+-~-+--+-~-+--+-~-+-\n");
    printf("\n[client] Menu:\n");
    printf("[client] list\n");
    printf("[client] upload <path>\n");
    printf("[client] download <filename>\n");
    printf("[client] get_sync_dir\n");
    printf("[client] exit\n");
    printf("[client]:~> ");

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

    printf("[client] command: [%s]\n", buffer);
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
        printf("[client] inotify_init error: %d [%s]\n", errno, strerror(errno));
    }

    wd = inotify_add_watch(fd, "/home/mauricio/mauricioa", IN_CREATE | IN_DELETE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO );
    if ( wd < 0 ) {
        printf("[client] inotify_add_watch error: %d [%s]\n", errno, strerror(errno));
    }

    //blocking
    length = read( fd, buffer, EVENT_BUF_LEN );
    if ( length < 0 ) {
        printf("[client] read error: %d [%s]\n", errno, strerror(errno));
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
                    printf("[client] *** %s ignored\n",event->name);
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
    // while(1)
    // {
    //     create_sync_inotify_home_folder();
    //     printf("[client] \n - \n");
    // }

    // int connect_server(char *host, int port);
    // Declara
    char username[USERNAME_SIZE];
    int sockfd;
    struct sockaddr_in serveraddr;
    char buffer[BUFFER_SIZE];

    // Inicializa
    memset(buffer, 0, BUFFER_SIZE);
    memset(username, 0, USERNAME_SIZE);

    // Cria
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1)
    {
        printf("[client] error opening socket\n");
        return 1;
    }
    int ServerIP[IP_SIZE];
    printf("[client] Enter with server IP: ");
    scanf("%s", ServerIP);
    
    printf("[client] socket created\n");

    // Configura
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ServerIP);
    serveraddr.sin_port = htons(SERVER_PORT);

    // Conecta
    if (connect(sockfd , (struct sockaddr *) &serveraddr , sizeof(serveraddr)) < 0)
    {
        printf("[client] error connecting to server\n");
        return 1;
    }

    // Envia HI
    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "HI");
    if(send(sockfd, buffer, BUFFER_SIZE, 0)  < 0)
    {
        printf("[client] Hi sent failed\n");
        return 1;
    }
    // printf("[client] Hi sent\n");

    // Recebe HI
    memset(buffer, 0, BUFFER_SIZE);
    if( recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        printf("[client] recv failed\n");
        return 1;
    }
    // printf("[client] Hi feedback received\n");

    // Envia username
    memset(username, 0, USERNAME_SIZE);
    printf("[client] username: ");
    scanf("%s", username);

    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "USERNAME");
    strcat(buffer, " ");
    strcat(buffer, username);
    strcat(buffer, "\0");
    if(send(sockfd, buffer, BUFFER_SIZE, 0)  < 0)
    {
        puts("[client] send failed.\n");
        return 1;
    }
    // printf("[client] username sent\n");

    // Recebe username
    memset(buffer, 0, BUFFER_SIZE);
    if( recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        printf("[client] recv failed.\n");
        return 1;
    }
    // printf("[client] username received\n");

    char* copy;
    char* command;

    // Pega dados do comando
    copy = strdup(buffer);
    command = strdup(strtok(copy, " "));


    if (strcmp(command, "NOTOK") == 0)
    {
        printf("[client] too many active sessions.\n");
        exit(0);
    }
    else if (strcmp(command, "SERVER") == 0)
    {
        printf("[client] server can't accept new connections.\n");
        exit(0);
    }

    printf("[client] connected\n");

    // home
    struct passwd* pw = getpwuid(getuid());
    char* homedir = strdup(pw->pw_dir);

    // Cria pasta sync local do usuário
    char folder[BUFFER_SIZE];
    strcpy(folder, homedir);
    strcat(folder, "/sync_dir_");
    strcat(folder, username);
    strcat(folder, "/");
    struct stat st = {0};
    mkdir(folder, 0700);
    printf("HOME=%s\n", folder);

    printf("[client] local dir verified\n");

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

                    // void close_connection();
                    close(sockfd);
                    return 0;

                break;

                case CMD_UPLOAD:

                    if(process_upload(sockfd, buffer) < 0)
                    {
                        printf("[client] upload failed.\n");
                        return -1;
                    } else {
                        printf("[client] upload completed.\n");
                    }

                break;

                case CMD_DOWNLOAD:

                    if(process_download(sockfd, buffer, username) < 0)
                    {
                        printf("[client] download failed.\n");
                        return -1;
                    } else {
                        printf("[client] download completed.\n");
                    }

                break;

                case CMD_SYNC:
                    // todo:

                break;

                default:

                    // Envia
                    if(send(sockfd, buffer, BUFFER_SIZE, 0)  < 0)
                    {
                        printf("[client] send command failed\n");
                        return 1;
                    }
                    // printf("[client] command [%s] sent.\n", buffer);

                    // Recebe
                    memset(buffer, 0, BUFFER_SIZE);
                    if( recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
                    {
                        printf("[client] recv failed\n");
                        return 1;
                    }
                    printf("[client] server response:\n%s\\\n", buffer);

                break;
        }

    }

    close(sockfd);

    return 0;
}
