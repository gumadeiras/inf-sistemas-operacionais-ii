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
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#define BUFFER_SIZE 1024
#define USERNAME_SIZE 100
#define IP_SIZE 15
#define NUM_MAX_FILES 10

#define CMD_ERROR -1
#define CMD_UPLOAD 1
#define CMD_EXIT 2
#define CMD_DOWNLOAD 3
#define CMD_SYNC_CLIENT 4
#define CMD_SYNC_SERVER 5
#define CMD_RMS 6

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN ( 1024 * ( EVENT_SIZE + 16 ) )

int shared_update = 0;
char shared_username[256];
int shared_socket;
int reconnect;
int position;
pthread_mutex_t lock;
int shared_inotify_isenabled = 10;
SSL *ssl;

struct file_info
{
    char* filename;
    char* file_lastmodified;
};
struct file_info file_info_array[NUM_MAX_FILES];

struct replica_manager
{
    char ip[IP_SIZE];
    int port;
};
struct replica_manager replica_array[NUM_MAX_FILES];

int enviar(int s, char* b, int size, int flags)
{
    // int r = send(s, b, size, flags);
    int r = SSL_write(ssl, b, size);
    // printf("********************** [client-sent] %s\n", b);
    return r;
}

int receber(int s, char* b, int size, int flags)
{
    // int r = recv(s, b, size, flags);
    int r = SSL_read(ssl, b, size);
    // printf("********************** [client-received] %s\n", b);
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
    printf("\nprocess_upload(): Hi\n");

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

    // printf("    [client] process_upload(): filename::%s\n",filename);
    // printf("    [client] process_upload(): filesize::%s\n",filesize);

    // Envia comando
    // printf("[client] ENVIA COMANDO: %s\n", message);
    enviar(sockfd, message, BUFFER_SIZE, NULL);

    // Envia arquivo
    remain_data = atoi(filesize);
    int j = 0;
    while (remain_data > 0 && (data_read = fread(&message, 1, BUFFER_SIZE, filefd)) > 0)
    {
        j++;
        int data_sent = enviar(sockfd, message, data_read, NULL);
        remain_data -= data_sent;
        printf("  process_upload(): j::%d dataread::%d datasent::%d remaindata::%d filesize::%s\n", j, data_read, data_sent, remain_data, filesize);
    }

    memset(message, 0, BUFFER_SIZE);
    receber(sockfd, message, BUFFER_SIZE, MSG_WAITALL);

    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "ACABOU FIM END");
    enviar(sockfd, message, BUFFER_SIZE, NULL);

    // Limpa a sujeira
    fclose(filefd);
    free(copy);

    printf("\nprocess_upload(): Bye\n");
}

// void receive_file(char *file);
int process_download(const int sockfd, const char* buffer, char* username)
{
    // Desativa inotify
    pthread_mutex_lock(&lock);
    shared_inotify_isenabled = 0;
    pthread_mutex_unlock(&lock);


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
        return -1;
    }

    // Homepath
    // struct passwd* pw = getpwuid(getuid());
    // char* homedir = strdup(pw->pw_dir);
    // syncpath = malloc(BUFFER_SIZE);
    // memset(syncpath, 0, BUFFER_SIZE);
    // strcpy(syncpath, homedir);
    // strcat(syncpath, "/sync_dir_");
    // strcat(syncpath, username);
    // strcat(syncpath, "/");
    // if(mkdir(syncpath, 0700) < 0)
    // {
    //     printf("[client] mkdir error (pasta ja existe) (errstr=%s) (errno=%d)\n", strerror(errno), errno);
    // }

    // Filepath
    // printf("%s\n", username);
    filepath = malloc(BUFFER_SIZE);
    memset(filepath, 0, BUFFER_SIZE);
    strcpy(filepath, "./");
    strcat(filepath, filename);
    // printf("FILEPATH: %s\n", filepath);

    // Cria arquivo
    filefd = fopen (filepath, "wb");
    if (filefd == NULL) {
        printf("[client] fopen error (errstr=%s) (errno=%d)\n", strerror(errno), errno);
        return -1;
    }

    // Recebe arquivo
    memset(message, 0, BUFFER_SIZE);
    int j = 0;
    while (remain_data > 0 && (data_read = receber(sockfd, message, BUFFER_SIZE, NULL)) > 0)
    {
        int data_write = fwrite(&message, 1, data_read, filefd);
        remain_data -= data_read;
        memset(message, 0, BUFFER_SIZE);
        j++;
        // printf("  recebendo[%d]: receber()::%d fwrite()::%d remaindata::%d filesize::%s\n", j, data_read, data_write, remain_data, filesize);
    }

    // Limpa a sujeira
    fclose(filefd);
    free(filepath);

    // Ativa inotify
    pthread_mutex_lock(&lock);
    shared_inotify_isenabled = 1;
    pthread_mutex_unlock(&lock);

    printf("[client] Download concluido\n", NULL);
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
            // printf("Waiting server permission...\n", NULL);
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

                int data_sent = enviar(sockfd, message, data_read, NULL);

                remain_data -= data_sent;

                memset(message, 0, BUFFER_SIZE);

                // printf("  dataread::%d data_sent::%d remaindata::%d filesize::%d j::%d \n", data_read, data_sent, remain_data, filesize, j);
            }
            sleep(1);
            fclose(filefd);
        }
    }
    closedir(pDir);

    // Recebe pedido
    // printf("Waiting server permission...\n", NULL);
    receber(sockfd, message, BUFFER_SIZE, MSG_WAITALL);

    // Avisa que terminou
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "FIM A B C D");
    enviar(sockfd, message, BUFFER_SIZE, NULL);

    // printf("get_sync_dir FINALIZADO\n", NULL);
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
    while (remain_data > 0 && (data_read = receber(sockfd, message, BUFFER_SIZE, NULL)) > 0)
    {
        int data_write = fwrite(&message, 1, data_read, filefd);

        remain_data -= data_read;

        memset(message, 0, BUFFER_SIZE);

        j++;

        // printf("  dataread::%d datawrite::%d remaindata::%d filesize::%d j::%d \n", data_read, data_write, remain_data, filesize, j);
    }

    fclose(filefd);
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
        // printf("*** WAITING INOTIFY ************************************\n", NULL);
        length = read(fd, buffer, EVENT_BUF_LEN);
        if (length < 0) {
            printf("[client] read error: %d [%s]\n", errno, strerror(errno));
        }

        // printf("*** INOTIFY LENGTH: %d\n", length);


        // Verifica se esta ativado
        pthread_mutex_lock(&lock);
        int isenabled = shared_inotify_isenabled;
        pthread_mutex_unlock(&lock);

        if(isenabled == 0)
        {
            // printf("isenabled == false! ignorando modificacoes");
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
                        // printf("[client] *** %s ignored\n",event->name);
                        i += EVENT_SIZE + event->len;
                        continue;
                    }
                }

                update = 10;

                if ( event->mask & IN_CREATE )
                {
                    if ( event->mask & IN_ISDIR )
                    {
                        // printf( "*** IN_CREATE DIR: %s\n", event->name );
                    }
                    else
                    {
                        // printf( "*** IN_CREATE FILE: %s\n", event->name );
                    }
                }
                else if ( event->mask & IN_DELETE )
                {
                    if ( event->mask & IN_ISDIR )
                    {
                        // printf( "*** IN_DELETE FILE: %s\n", event->name );
                    }
                    else
                    {
                        // printf( "*** IN_DELETE DIR: %s\n", event->name );
                    }
                }
                else if ( event->mask & IN_MODIFY )
                {
                    if ( event->mask & IN_ISDIR )
                    {
                        // printf( "*** IN_MODIFY DIR: %s\n", event->name );
                    }
                    else
                    {
                        // printf( "*** IN_MODIFY FILE: %s\n", event->name );
                    }
                }
                else if ( event->mask & IN_CLOSE_WRITE )
                {
                    if ( event->mask & IN_ISDIR )
                    {
                        // printf( "*** IN_CLOSE_WRITE DIR: %s\n", event->name );
                    }
                    else
                    {
                        // printf( "*** IN_CLOSE_WRITE FILE: %s\n", event->name );
                    }
                }
                else if ( event->mask & IN_MOVED_TO )
                {
                    if ( event->mask & IN_ISDIR )
                    {
                        // printf( "*** IN_MOVED_TO DIR: %s\n", event->name );
                    }
                    else
                    {
                        // printf( "*** IN_MOVED_TO FILE: %s\n", event->name );
                    }
                }
                else if ( event->mask & IN_MOVED_FROM )
                {
                    if ( event->mask & IN_ISDIR )
                    {
                        // printf( "*** IN_MOVED_FROM DIR: %s\n", event->name );
                    }
                    else
                    {
                        // printf( "*** IN_MOVED_FROM FILE: %s\n", event->name );
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
    char username[USERNAME_SIZE];
    struct sockaddr_in serveraddr;
    char buffer[BUFFER_SIZE];
    char server_ip[IP_SIZE];
    char port[IP_SIZE];
    int server_port;

    if (reconnect == 1)
    {
        pthread_mutex_lock(&lock);
        strcpy(server_ip, replica_array[position].ip);
        server_port = replica_array[position].port;
        pthread_mutex_unlock(&lock);
        position++;
    } else {
    printf("[client] enter server ip: ");
    scanf("%s", server_ip);

    printf("[client] enter server port: ");
    scanf("%s", port);
    server_port = atoi(port);
    }

    memset(buffer, 0, BUFFER_SIZE);
    memset(username, 0, USERNAME_SIZE);

    // Cria
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd < 0)
    {
        printf("[client] error opening socket\n", NULL);
        return 1;
    }

    // Configura
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(server_ip);
    serveraddr.sin_port = htons(server_port);

    // Conecta
    if (connect(sockfd , (struct sockaddr *) &serveraddr , sizeof(serveraddr)) < 0)
    {
        printf("[client] error connecting to server\n", NULL);
        return 1;
    }

    shared_socket = sockfd;

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    SSL_METHOD *method;
    SSL_CTX *ctx;
    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        abort();
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, shared_socket);
    if (SSL_connect(ssl) == -1)
        ERR_print_errors_fp(stderr);
    else {
        //GG
        X509 *cert;
        char *line;
        cert = SSL_get_peer_certificate(ssl);
        if (cert != NULL) {
            printf("[client] Client certificates:\n");
            line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
            printf("[client] Subject: %s\n", line);
            free(line);
            line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
            printf("[client] Issuer: %s\n", line);
            free(line);
            X509_free(cert);
        }
        else
            printf("[client] No certificates.\n");
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
    if(receber(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL) < 0)
    {
        printf("[client] HI recv failed\n", NULL);
        return 1;
    }

    // Envia username
    if (reconnect == 1)
    {
        strcpy(username, shared_username);
    } else {
        memset(username, 0, USERNAME_SIZE);
        printf("[client] username: ", NULL);
        scanf("%s", username);
    }

    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "USERNAME");
    strcat(buffer, " ");
    strcat(buffer, username);
    strcat(buffer, "\0");
    if (enviar(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        puts("[client] send failed.\n");
        return 1;
    }

    // Recebe username
    memset(buffer, 0, BUFFER_SIZE);
    if( receber(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL) < 0)
    {
        printf("[client] USERNAME recv failed.\n", NULL);
        return 1;
    }

    char* copy;
    char* command;

    // Pega dados do comando
    copy = strdup(buffer);

    if (position%2 == 0)
        process_rms(buffer);

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

    reconnect = 0;
    return sockfd;
}

int process_rms(const char* buffer)
{
    int i;
    char* copy;
    char ip[IP_SIZE];
    char port[IP_SIZE];
    copy = strdup(buffer);
    // command
    strcpy(ip, strdup(strtok(copy, " ")));

    // for (i = 0; i < NUM_MAX_FILES; ++i)
    // {
    // ip1
    if (ip == '\0')
    {
        return 0;
    }
    strcpy(ip, strdup(strtok(NULL, " ")));
    strcpy(replica_array[position].ip, ip);
    // port1
    strcpy(port, strdup(strtok(NULL, " ")));
    replica_array[position].port = atoi(port);
    // }
}

// Baixa todos os arquivos do servidor
int process_sync_client(const int sockfd, const char* buffer, char* username)
{
    // printf("\n process_sync_client(): Hi \n");

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
    // printf("process_sync_client(): homepath::%s\n", folderpath);


    // Deleta tudo
    deleteAllFiles(folderpath);
    // printf("process_sync_client(): homepath files deleted\n", NULL);


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
    // printf("process_sync_client(): enviando::%s\n",message);


    // Recebe arquivos
    while(1)
    {


        // Pede um
        memset(message, 0, BUFFER_SIZE);
        strcpy(message, "NEXT FILE");
        enviar(sockfd, message, BUFFER_SIZE, NULL);
        // printf("process_sync_client(): enviado::%s\n",message);


        // Ganha um
        memset(message, 0, BUFFER_SIZE);
        ret = receber(sockfd, message, BUFFER_SIZE, MSG_WAITALL);
        // printf("process_sync_client(): recebido::%s\n",message);
        char* arg1 = strdup(strtok(message, " "));


        // Analisa
        if(strcmp(arg1, "FILE") == 0)
        {
            // printf("\n process_sync_client(): FILE START \n");

            char* filename = strdup(strtok(NULL, " "));
            char* filesize = strdup(strtok(NULL, " "));

            char filepath[1024];
            strcpy(filepath, folderpath);
            strcat(filepath, filename);

            // printf("process_sync_client(): recebendo arquivo [%s] [%s bytes]\n", filename, filesize);

            receive_one_file(sockfd, filepath, filename, atoi(filesize));

            free(filename);
            free(filesize);

            // printf("\n process_sync_client(): FILE END \n");
        }
        else if(strcmp(arg1, "FIM") == 0)
        {
            // printf("\n process_sync_client(): FIM \n");
            break;
        }
    }

    // Ativa inotify
    pthread_mutex_lock(&lock);
    shared_inotify_isenabled = 10;
    pthread_mutex_unlock(&lock);

    // printf("\n process_sync_client(): Bye \n");
}

void* server_sync_thread(void* thread_function_arg)
{
    char buffer[BUFFER_SIZE];
    while(1)
    {
    // printf("THREADS\n");
    sleep(10);
    if(check_if_server_has_changed(shared_socket, shared_username) == 3)
        {
        // printf("VAI SYNC\n");
            process_sync_client(shared_socket, buffer, shared_username);
        }
    }
}

void* client_sync_thread(void* thread_function_arg)
{
    char buffer[BUFFER_SIZE];
    while(1)
    {
    // printf("THREADS\n");
    sleep(15);
    if(shared_update)
        {
            // Upload all files to server
            process_sync_server(shared_socket, shared_username);
            // getTimeServer(buffer);
            // Para nao repetir a sincronizacao
            pthread_mutex_lock(&lock);
            shared_update = 0;
            pthread_mutex_unlock(&lock);
        }
    }
}

int getTimeServer(const char* buffer)
{
    // Variaveis para sincronizar lastmodified com server
    time_t timesent, timereceived, servertime, final;
    FILE* filefd;
    char* serverT;
    char* copy;
    char* command;
    char* filepath;
    char* filename;
    char  filesize[50];
    char  message[BUFFER_SIZE];

    // Pega dados do comando que estao separados por um espaco
    copy = strdup(buffer);
    command = strdup(strtok(copy, " "));
    filepath = strdup(strtok(NULL, " "));

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

    // Monta comando para perguntar a hora do server
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "gettime ");

    // Pega hora de envio
    time(&timesent);

    // Pega hora do server
    enviar(shared_socket, message, BUFFER_SIZE, NULL);
    serverT = receber(shared_socket, message, BUFFER_SIZE, NULL);
    // Pega hora da resposta
    time(&timereceived);

    servertime = atoi(serverT);

    // Algoritmo de cristian simplificado para sincronizar tempo
    final = (servertime + ((timereceived - timesent) / 2));

    //procura arquivo na lista de arquivos e altera horário
    int i;
    for (i = 0; i < NUM_MAX_FILES; ++i)
    {
        pthread_mutex_lock(&lock);
        char* fn = file_info_array[i].filename;
        pthread_mutex_unlock(&lock);
        if (strcmp(fn, filename) == 0)
        {
            pthread_mutex_lock(&lock);
            file_info_array[i].file_lastmodified = final;
            pthread_mutex_unlock(&lock);
        }
    }

    fclose(filefd);
}

int check_if_server_has_changed(int sockfd, char* username)
{
    // printf("\n check_if_server_has_changed(): Hi \n");

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "newfiles");
    strcat(buffer, " ");
    strcat(buffer, username);
    strcat(buffer, " ");
    strcat(buffer, "end");
    // printf("check_if_server_has_changed(): enviandooo::%s\n", buffer);
    if(enviar(sockfd, buffer, BUFFER_SIZE, 0)  <= 0)
    {
        printf("[client] check_if_server_has_changed failed\n", NULL);
        reconnect = 1;
        conecta();
        return 0;
    }

    memset(buffer, 0, BUFFER_SIZE);
    // printf("recebendoooo\n");
    if( receber(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL) <= 0)
    {
        printf("[client] check_if_server_has_changed recv failed\n", NULL);
        reconnect = 1;
        conecta();
        return 0;
    }

    char* command = strdup(strtok(buffer, " "));
    // printf("recebeu: %s\n", command);
    if (strcmp(command, "NOTOK") == 0)
    {
        // printf("check_if_server_has_changed: PRECISA SINCRONIZAR\n");
        return 3;
    }
    else if (strcmp(command, "OK") == 0)
    {
        // printf("check_if_server_has_changed: NAO PRECISA SINCRONIZAR\n");
    }

    // printf("\n check_if_server_has_changed(): Bye \n");

    return 0;
}

int main()
{
    pthread_t inotify_thread_data;
    pthread_t server_sync;
    pthread_t client_sync;
    int inotify_thread_number;
    int sockfd;
    char buffer[BUFFER_SIZE];
    char* syncpath;

    // Inicia conexao
    reconnect = 0;
    position = 0;
    sockfd = conecta();


    // Seta local username from shared username
    char username[1024];
    memset(username, 0 , 1024);
    pthread_mutex_lock(&lock);
    strcpy(username, shared_username);
    pthread_mutex_unlock(&lock);


    // Checa se homepath existe
    struct passwd* pw = getpwuid(getuid());
    char* homedir = strdup(pw->pw_dir);
    syncpath = malloc(BUFFER_SIZE);
    memset(syncpath, 0, BUFFER_SIZE);
    strcpy(syncpath, homedir);
    strcat(syncpath, "/sync_dir_");
    strcat(syncpath, username);
    strcat(syncpath, "/");
    mkdir(syncpath, 0700);

    // Inicia inotify
    inotify_thread_number = pthread_create(&inotify_thread_data, NULL, inotify_thread_function, (void*) NULL);
    if (inotify_thread_number < 0)
    {
        printf("[client] pthread_create failed (errstr=%s) (errno=%d)\n", strerror(errno), errno);
        return -1;
    }

    pthread_create(&server_sync, NULL, server_sync_thread, (void*) NULL);
    // pthread_create(&client_sync, NULL, client_sync_thread, (void*) NULL);

    // Start the loop
    while(1)
    {

        // Verifica se usuario modificou home folder
        if(shared_update)
        {
            // Upload all files to server
            process_sync_server(sockfd, username);

            // Para nao repetir a sincronizacao
            pthread_mutex_lock(&lock);
            shared_update = 0;
            pthread_mutex_unlock(&lock);
        }


        // Verifica se servidor recebeu algum arquivo de outra instancia
        // if(check_if_server_has_changed(sockfd, username) == 3)
        // {
        //     // Download todos files do server
        //     process_sync_client(sockfd, buffer, username);
        // }

        // Pega um comando do usuario e processa
        if(fill_buffer_with_command(buffer, BUFFER_SIZE) < 0)
        {
            // Tenta de novo
            continue;
        }
        switch(get_command_from_buffer(buffer))
        {
            case CMD_EXIT:
                close(sockfd);
                return 0;
            break;

            case CMD_UPLOAD:
                if(process_upload(sockfd, buffer) <= 0)
                {
                    printf("[client] upload failed.\n", NULL);
                    return -1;
                } else {
                    printf("[client] upload completed.\n", NULL);
                }
            break;

            case CMD_DOWNLOAD:
                if(process_download(sockfd, buffer, username) <= 0)
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
                if(enviar(sockfd, buffer, BUFFER_SIZE, 0) <= 0)
                {
                    printf("[client] send failed\n", NULL);
                    return 1;
                }
                memset(buffer, 0, BUFFER_SIZE);
                if( receber(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL) <= 0)
                {
                    printf("[client] GET_COMMAND_FROM_BUFFER recv failed\n", NULL);
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
