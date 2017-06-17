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
#include <pthread.h>
#include <pwd.h>

#define BUFFER_SIZE 1024
#define USERNAME_SIZE 100
#define SERVER_PORT 3002

#define CMD_ERROR -1
#define CMD_UPLOAD 1
#define CMD_EXIT 2
#define CMD_DOWNLOAD 3
#define CMD_SYNC_CLIENT 4
#define CMD_SYNC_SERVER 5

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN ( 1024 * ( EVENT_SIZE + 16 ) )

int shared_update = 0;
char shared_username[256];
pthread_mutex_t lock;
int shared_inotify_isenabled = 10;

int enviar(int s, char* b, int size, int flags)
{
    int r = send(s, b, size, flags);
    printf("********************** [client-sent] %s\n", b);
    return r;
}

int receber(int s, char* b, int size, int flags)
{
    int r = recv(s, b, size, flags);
    printf("********************** [client-received] %s\n", b);
    return r;
}


void texto(char* string, va_list argp)
{
    printf(string, argp);
}

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

    if (strcmp(firstString, "get_sync_dir") == 0)
    {
        command = CMD_SYNC_CLIENT;
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
        printf("[client] Nao pode abrir arquivo\n",NULL);
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
    enviar(sockfd, message, BUFFER_SIZE, NULL);

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
    enviar(sockfd, message, BUFFER_SIZE, NULL);

    // Espera resposta
    memset(message, 0, BUFFER_SIZE);
    receber(sockfd, message, BUFFER_SIZE, MSG_WAITALL);

    // Trata resposta
    copy2 = strdup(message);
    command2 = strdup(strtok(copy2, " "));
    filename2 =  strdup(strtok(NULL, " "));
    filesize =  strdup(strtok(NULL, " "));
    remain_data = atoi(filesize);

    // Verifica erro
    if (strcmp(command, "ERROR") == 0)
    {
        printf("[client] ERROR!\n", NULL);
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
    while (remain_data > 0 && (data_read = recv(sockfd, message, BUFFER_SIZE, MSG_WAITALL)) > 0)
    {
        fwrite(&message, 1, data_read, filefd);
        remain_data -= data_read;
        printf("[client] RECV(): [%d/%d - %d]\n", remain_data, atoi(filesize), data_read);
        memset(message, 0, BUFFER_SIZE);
    }
    printf("[client] transfer completed\n", NULL);

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
    /*
    printf("\n\n\n-+-~-+--+-~-+--+-~-+--+-~-+--+-~-+--+-~-+--+-~-+--+-~-+-\n");
    printf("\n[client] Menu:\n");
    printf("[client] list\n");
    printf("[client] upload <path>\n");
    printf("[client] download <filename>\n");
    printf("[client] get_sync_dir\n");
    printf("[client] exit\n");
    */
    printf("[client]:~> ", NULL);

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

// Envia tudo para o server
int process_sync_server(int sockfd, char* username)
{
    // deleta tudo no server

    // envia tudo para server
    DIR* pDir;
    struct dirent* pDirent;
    int ret;
    

    // Get home user folder
    char folderpath[BUFFER_SIZE];
    struct passwd* pw = getpwuid(getuid());
    memset(folderpath, 0, BUFFER_SIZE);
    strcpy(folderpath, pw->pw_dir);
    strcat(folderpath, "/sync_dir_");
    strcat(folderpath, username);
    strcat(folderpath, "/");


    // Envia comando para o server
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "SYNCSERVER");
    strcat(message," ");
    strcat(message,"arg1");
    ret = enviar(sockfd, message, BUFFER_SIZE, NULL);
    if(ret < 0)
    {
        printf("process_sync_client: send error\n", NULL);
    }


    // Envia arquivos
    pDir = opendir(folderpath);
    if (pDir == NULL) {
        printf("process_sync_client: opendir error :(\n", NULL);
        return -1;
    }
    while ((pDirent = readdir(pDir)) != NULL) 
    {
        if(pDirent->d_type == DT_REG)
        {
            FILE* filefd;
            int data_read = 0;
            int remain_data = 0;

            char message[BUFFER_SIZE];

            char filepath[1024];
            char filename[512];
            char filesizes[512];
            int filesize;


            // Filepath
            strcpy(filename, pDirent->d_name);
            strcpy(filepath, folderpath);
            strcat(filepath, "/");
            strcat(filepath, filename);


            // Recebe pedido de um
            printf("Waiting server permission...\n", NULL);
            receber(sockfd, message, BUFFER_SIZE, MSG_WAITALL);


            // Envia um
            filefd = fopen (filepath, "rb");
            if (filefd == NULL) {
                printf("process_sync_client: fopen error :(\n", NULL);
                return -1;
            }


            // Filesize
            fseek(filefd, 0L, SEEK_END);
            sprintf(filesizes, "%ld", ftell(filefd));
            rewind(filefd);
            remain_data = atoi(filesizes);
            filesize = atoi(filesizes);


            // Monta comando
            memset(message, 0, BUFFER_SIZE);
            strcpy(message, "FILE");
            strcat(message," ");
            strcat(message, filename);
            strcat(message," ");
            strcat(message, filesizes);


            // Envia comando
            ret = enviar(sockfd, message, BUFFER_SIZE, NULL);
            if(ret < 0)
            {
                printf("process_sync_client: send error :(\n", NULL);
            }


            // Envia arquivo
            memset(message, 0, BUFFER_SIZE);
            remain_data = atoi(filesizes);
            int j =0;
            while (remain_data > 0 && (data_read = fread(&message, 1, BUFFER_SIZE, filefd)) > 0)
            {
                j++;

                int data_sent = send(sockfd, message, data_read, NULL);

                remain_data -= data_sent;

                memset(message, 0, BUFFER_SIZE);

                printf("  dataread::%d data_sent::%d remaindata::%d filesize::%d j::%d \n", data_read, data_sent, remain_data, filesize, j);
            }
            sleep(1);
            fclose(filefd);
        }
    }
    closedir(pDir);

    // Avisa que terminou
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "FIM A B C D");
    write(sockfd, message, BUFFER_SIZE);

    printf("get_sync_dir FINALIZADO\n", NULL);
}

int receive_one_file(int sockfd, char* filepath, char* filename, int filesize)
{
    FILE* filefd;
    int remain_data = filesize;
    int data_read = 0;
    char message[BUFFER_SIZE];

    filefd = fopen(filepath, "wb");
    if (filefd == NULL) 
    {
        printf("fopen error\n", NULL);
        return -1;
    }

    memset(message, 0, BUFFER_SIZE);
    int j = 0;
    while (remain_data > 0 && (data_read = recv(sockfd, message, BUFFER_SIZE, NULL)) > 0)
    {
        int data_write = fwrite(&message, 1, data_read, filefd);

        remain_data -= data_read;

        memset(message, 0, BUFFER_SIZE);

        j++;

        printf("  dataread::%d datawrite::%d remaindata::%d filesize::%d j::%d \n", data_read, data_write, remain_data, filesize, j);
    }

    fclose(filefd);
}

// Pega todos os arquivos do servidor
int process_sync_client(const int sockfd, const char* buffer, char* username)
{
    printf("process_sync_client(): iniciando\n", NULL);

    int ret;


    // Desativa inotify
    pthread_mutex_lock(&lock);
    shared_inotify_isenabled = 0;
    pthread_mutex_unlock(&lock);

    
    // Get home user folder
    char folderpath[BUFFER_SIZE];
    struct passwd* pw = getpwuid(getuid());
    memset(folderpath, 0, BUFFER_SIZE);
    strcpy(folderpath, pw->pw_dir);
    strcat(folderpath, "/sync_dir_");
    strcat(folderpath, username);
    strcat(folderpath, "/");
    printf("process_sync_client(): homepath::%s\n", folderpath);


    // Deleta tudo
    deleteAllFiles(folderpath);
    printf("process_sync_client(): homepath files deleted\n", NULL);


    // Envia comando para server
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "syncclient");
    strcat(message," ");
    strcat(message,"arg1");
    ret = enviar(sockfd, message, BUFFER_SIZE, NULL);
    if(ret < 0)
    {
        printf("process_sync_client: send error\n", NULL);
    }
    printf("process_sync_client(): enviando::%s\n",message);


    // Recebe arquivos
    while(1)
    {
        

        // Pede um
        memset(message, 0, BUFFER_SIZE);
        strcpy(message, "NEXT FILE");
        enviar(sockfd, message, BUFFER_SIZE, NULL);
        printf("process_sync_client(): enviado::%s\n",message);


        // Ganha um
        memset(message, 0, BUFFER_SIZE);
        ret = receber(sockfd, message, BUFFER_SIZE, MSG_WAITALL);
        printf("process_sync_client(): recebido::%s\n",message);
        char* arg1 = strdup(strtok(message, " "));


        // Analisa
        if(strcmp(arg1, "FILE") == 0)
        {
            char* filename = strdup(strtok(NULL, " "));
            char* filesize = strdup(strtok(NULL, " "));
            
            char filepath[1024];
            strcpy(filepath, folderpath);
            strcat(filepath, filename);

            printf("process_sync_client(): recebendo arquivo [%s] [%s bytes]\n", filename, filesize);

            receive_one_file(sockfd, filepath, filename, atoi(filesize));

            free(filename);
            free(filesize);
        }
        else if(strcmp(arg1, "FIM") == 0)
        {
            break;
        }
    }

    // Ativa inotify
    pthread_mutex_lock(&lock);
    shared_inotify_isenabled = 10;
    pthread_mutex_unlock(&lock);
}

void* inotify_thread_function(void* thread_function_arg)
{
    int i;
    int fd;
    int wd;
    int length;
    char buffer[EVENT_BUF_LEN];
    int update = 0;

    // Get home user folder
    char folderpath[BUFFER_SIZE];
    struct passwd* pw = getpwuid(getuid());
    memset(folderpath, 0, BUFFER_SIZE);
    strcpy(folderpath, pw->pw_dir);
    strcat(folderpath, "/sync_dir_");

    pthread_mutex_lock(&lock);
    strcat(folderpath, shared_username);
    pthread_mutex_unlock(&lock);
    
    strcat(folderpath, "/");

    fd = inotify_init();
    if ( fd < 0 ) {
        printf("[client] inotify_init error: %d [%s]\n", errno, strerror(errno));
    }

    wd = inotify_add_watch(fd, folderpath, IN_CREATE | IN_DELETE | IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_FROM | IN_MOVED_TO );
    if ( wd < 0 ) {
        printf("[client] inotify_add_watch error: %d [%s]\n", errno, strerror(errno));
    }
    
    while(1)
    {
        // blocking
        printf("*** WAITING INOTIFY ************************************\n", NULL);
        length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            printf("[client] read error: %d [%s]\n", errno, strerror(errno));
        }

        printf("*** INOTIFY LENGTH: %d\n", length);


        // Verifica se esta ativado
        pthread_mutex_lock(&lock);
        int isenabled = shared_inotify_isenabled;
        pthread_mutex_unlock(&lock);

        if(!isenabled)
        {
            continue;
        }
        

        i = 0;
        update = 0;

        for ( ; i < length; )
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

                update = 10;

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

        if(update) 
        {
            pthread_mutex_lock(&lock);
            shared_update = 10;
            pthread_mutex_unlock(&lock);
        }
    }

    inotify_rm_watch( fd, wd );

    close(fd);
}

int conecta()
{
    int sockfd;

    // Client
    char username[USERNAME_SIZE];
    
    struct sockaddr_in serveraddr;
    char buffer[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE);
    memset(username, 0, USERNAME_SIZE);

    // Cria
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1)
    {
        printf("[client] error opening socket\n", NULL);
        return 1;
    }
    printf("[client] socket created\n", NULL);

    // Configura
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serveraddr.sin_port = htons(SERVER_PORT);

    // Conecta
    if (connect(sockfd , (struct sockaddr *) &serveraddr , sizeof(serveraddr)) < 0)
    {
        printf("[client] error connecting to server\n", NULL);
        return 1;
    } 

    // Envia HI
    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "HI");
    if(enviar(sockfd, buffer, BUFFER_SIZE, 0)  < 0)
    {
        printf("[client] Hi sent failed\n", NULL);
        return 1;
    }

    // Recebe HI
    memset(buffer, 0, BUFFER_SIZE);
    if( receber(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL) < 0)
    {
        printf("[client] recv failed\n", NULL);
        return 1;
    }

    // Envia username
    memset(username, 0, USERNAME_SIZE);
    printf("[client] username: ", NULL);
    scanf("%s", username);

    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "USERNAME");
    strcat(buffer, " ");
    strcat(buffer, username);
    strcat(buffer, "\0");
    if(enviar(sockfd, buffer, BUFFER_SIZE, 0)  < 0)
    {
        puts("[client] send failed.\n");
        return 1;
    }

    // Recebe username
    memset(buffer, 0, BUFFER_SIZE);
    if( receber(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL) < 0)
    {
        printf("[client] recv failed.\n", NULL);
        return 1;
    }

    char* copy;
    char* command;

    // Pega dados do comando
    copy = strdup(buffer);
    command = strdup(strtok(copy, " "));

    if (strcmp(command, "NOTOK") == 0)
    {
        printf("[client] too many active sessions.\n", NULL);
        exit(0);
    }
    else if (strcmp(command, "SERVER") == 0)
    {
        printf("[client] server can't accept new connections.\n", NULL);
        exit(0);
    }

    pthread_mutex_lock(&lock);
    strcpy(shared_username, username);
    pthread_mutex_unlock(&lock);

    free(command);

    printf("[client] connected!\n", NULL);

    return sockfd;
}

int check_if_server_has_changed(int sockfd, char* username)
{
    return 0;
}

int main()
{
    pthread_t inotify_thread_data;
    int inotify_thread_number;
    int sockfd;
    char buffer[BUFFER_SIZE];


    // Inicia conexao
    sockfd = conecta();


    // Inicia inotify
    inotify_thread_number = pthread_create(&inotify_thread_data, NULL, inotify_thread_function, (void*) NULL);
    if (inotify_thread_number < 0)
    {
        printf("[client] pthread_create failed (errstr=%s) (errno=%d)\n", strerror(errno), errno);
        return -1;
    }


    // Seta local username from shared username
    char username[1024];
    memset(username, 0 , 1024);
    pthread_mutex_lock(&lock);
    strcpy(username, shared_username);
    pthread_mutex_unlock(&lock);
    

    // Checa se homepath existe
    struct passwd* pw = getpwuid(getuid());
    char* homedir = strdup(pw->pw_dir);
    char folder[BUFFER_SIZE];
    strcpy(folder, homedir);
    strcat(folder, "/sync_dir_");
    strcpy(folder, username);
    strcat(folder, "/");
    struct stat st = {0};
    mkdir(folder, 0700);


    // Start the loop
    while(1)
    {
        // Verifica se usuario modificou home folder
        if(shared_update)
        {
            printf("Wait, updating server...", NULL);
            process_sync_server(sockfd, username);
            pthread_mutex_lock(&lock);
            shared_update = 0;
            pthread_mutex_unlock(&lock);
        }


        // Verifica se servidor recebeu algum arquivo de outra instancia
        else if(check_if_server_has_changed(sockfd, username))
        {
            printf("Wait, updating this client...", NULL);
        }


        // Pega um comando do usuario e processa
        else if(fill_buffer_with_command(buffer, BUFFER_SIZE) < 0)
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
                    printf("[client] upload failed.\n", NULL);
                    return -1;
                } else {
                    printf("[client] upload completed.\n", NULL);
                }
            break;

            case CMD_DOWNLOAD:
                if(process_download(sockfd, buffer, username) < 0)
                {
                    printf("[client] download failed.\n", NULL);
                    return -1;
                } else {
                    printf("[client] download completed.\n", NULL);
                }
            break;

            case CMD_SYNC_CLIENT:
                process_sync_client(sockfd, buffer, username);
            break;

            default:
                if(enviar(sockfd, buffer, BUFFER_SIZE, 0)  < 0)
                {
                    printf("[client] send failed\n", NULL);
                    return 1;
                }
                memset(buffer, 0, BUFFER_SIZE);
                if( receber(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL) < 0)
                {
                    printf("[client] recv failed\n", NULL);
                    return 1;
                }
                printf("[client] server response:\n%s\\\n", buffer);
            break;
        }
    }


    // Finaliza
    close(sockfd);
    return 0;
}