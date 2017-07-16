//*
//* Trabalho 1
//* Instituto de Informática - UFRGS
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxServer.c
//* implementação das funções do cliente
//*

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

// Server config
#define IP_SIZE 15
#define NUM_MAX_CLIENT 10

// Internal use
#define BUFFER_SIZE 1024
#define USERNAME_SIZE 100
#define PATH_SIZE 2048

#define CMD_ERROR -1
#define CMD_HI 1
#define CMD_USERNAME 2
#define CMD_LIST 3
#define CMD_RECV 4
#define CMD_SEND 5
#define CMD_SYNC_CLIENT 6
#define CMD_SYNC_SERVER 7
#define CMD_NEWFILES 8
#define CMD_GETTIME 9
#define CMD_RMSYNC 10

struct client_info
{
    int client_id;
    int client_number;
    struct sockaddr_in client_info;
    char folderPath[PATH_SIZE];
    char isActive;
    char username[USERNAME_SIZE];
    int new_files;
    SSL *ssl;
};

struct file_info
{
    char* filename;
    char* file_lastmodified;
};

struct client_info client_info_array[NUM_MAX_CLIENT]; // SHARED VARIABLE ONE
pthread_mutex_t lock; // SHARED-VARIABLE-ONE LOCK
char server_ip[IP_SIZE];
int server_port;
char rm_ip[IP_SIZE];
char port[IP_SIZE];
int rm_port;
SSL *rmssl;

int enviar(SSL *ssl, char* b, int size, int flags)
{
    // int r = send(s, b, size, flags);
    int r = SSL_write(ssl, b, size);
    printf("[client-sent] %s\n", b);
    return r;
}

int receber(SSL *ssl, char* b, int size, int flags)
{
    // int r = recv(s, b, size, flags);
    int r = SSL_read(ssl, b, size);
    printf("[client-received] %s\n", b);
    return r;
}

char toLowercase(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? (ch + 32) : (ch);
}

void turnStringLowercase(char* string)
{
    int index;
    for (index=0; string[index]; index++)
    {
        string[index] = toLowercase(string[index]);
    }
}

int getCommand(const char* string)
{
    printf("\n getCommand(): Hi \n");

    char* copy = strdup(string);
    char* firstString = strtok(copy, " ");

    int command = CMD_ERROR;

    printf("getCommand:%s\n", firstString);
    turnStringLowercase(firstString);

    if (strcmp(firstString, "hi") == 0)
    {
        command = CMD_HI;
    }

    if (strcmp(firstString, "username") == 0)
    {
        command = CMD_USERNAME;
    }

    if (strcmp(firstString, "list") == 0)
    {
        command = CMD_LIST;
    }

    if (strcmp(firstString, "upload") == 0)
    {
        command = CMD_RECV;
    }

    if (strcmp(firstString, "download") == 0)
    {
        command = CMD_SEND;
    }

    if (strcmp(firstString, "syncclient") == 0)
    {
        command = CMD_SYNC_CLIENT;
    }

    if (strcmp(firstString, "syncserver") == 0)
    {
        command = CMD_SYNC_SERVER;
    }
    if (strcmp(firstString, "newfiles") == 0)
    {
        command = CMD_NEWFILES;
    }
    if (strcmp(firstString, "gettime") == 0)
    {
        command = CMD_GETTIME;
    }
    if (strcmp(firstString, "rmsync") == 0)
    {
        command = CMD_RMSYNC;
    }

    free(copy);

    printf("\n getCommand(): Bye \n");

    return command;
}

 // void receive_file(char *file);

// client envia "upload dog.jpg"
int process_recv(SSL *sockfd, const char* buffer, int id)
{
    printf("\nprocess_recv(): Hi\n");

    // Declara e Inicializa
    char message[BUFFER_SIZE];

    FILE* filefp;
    int data_read;
    int remain_data;

    char* copy;
    char* command;
    char* filename;
    char* filesize;
    char* filepath;

    // Pega dados do comando separados por espaco
    copy = strdup(buffer);
    command = strdup(strtok(copy, " "));
    filename =  strdup(strtok(NULL, " "));
    filesize =  strdup(strtok(NULL, " "));

    // Monta o path do arquivo
    filepath = malloc(2048);
    memset(filepath, 0, 2048);
    pthread_mutex_lock(&lock);
    strcpy(filepath, client_info_array[id].folderPath);
    client_info_array[id].new_files = 1;
    pthread_mutex_unlock(&lock);
    strcat(filepath, "/");
    strcat(filepath, filename);

    // Cria arquivo
    filefp = fopen (filepath, "wb");
    if (filefp == NULL) {
        printf("[server] Error: %d [%s]\n", errno, strerror(errno));
        return -1;
    }

    // Recebe arquivo
    memset(message, 0, BUFFER_SIZE);
    remain_data = atoi(filesize);
    int j = 0;
    while (remain_data > 0 && (data_read = receber(sockfd, message, BUFFER_SIZE, NULL)) > 0)
    {
        int datawrite = fwrite(&message, 1, data_read, filefp);
        remain_data -= data_read;
        j++;
        memset(message, 0, BUFFER_SIZE);

        printf("  process_recv(): j::%d dataread::%d datawrite::%d remaindata::%d filesize::%s\n", j, data_read, datawrite, remain_data, filesize);
    }
    printf("\nprocess_recv(): Upload complete\n");

    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "ACABOU TUDO END");
    enviar(sockfd, message, BUFFER_SIZE, NULL);

    memset(message, 0, BUFFER_SIZE);
    receber(sockfd, message, BUFFER_SIZE, MSG_WAITALL);

    // Limpa a sujeira
    fclose(filefp);
    free(filepath);
    free(copy);

    printf("\nprocess_recv(): Bye\n");
}

// void receive_file(char *file);
int process_send(SSL *sockfd, const char* buffer, int id)
{
    // Declara e Inicializa
    char message[BUFFER_SIZE];

    FILE* filefd;
    int data_read;
    int remain_data = 0;

    char* copy;
    char* command;
    char* filepath;
    char* filename;
    char filesize[50];

    // Recebe dados do comando
    copy = strdup(buffer);
    command = strdup(strtok(copy, " "));
    filename =  strdup(strtok(NULL, " "));

    // Monta o path do arquivo que o cliente quer receber
    filepath = malloc(2048);
    pthread_mutex_lock(&lock);
    strcpy(filepath, client_info_array[id].folderPath);
    pthread_mutex_unlock(&lock);
    strcat(filepath, "/");
    strcat(filepath, filename);

    // Verica se o arquivo existe
    filefd = fopen (filepath, "rb");
    if (filefd == NULL) {
        printf("[server] error: %d [%s]\n", errno, strerror(errno));
        printf("[server] file not found\n");
        memset(message, 0, BUFFER_SIZE);
        strcpy(message, "ERROR FILE NOT FOUND");
        enviar(sockfd, message, BUFFER_SIZE, NULL);
        return -1;
    }

    // Pega filesize
    fseek(filefd, 0L, SEEK_END);
    sprintf(filesize, "%ld", ftell(filefd));
    rewind(filefd);

    // Monta comando
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "sending");
    strcat(message," ");
    strcat(message, filename);
    strcat(message," ");
    strcat(message, filesize);

    // Envia comando
    enviar(sockfd, message, BUFFER_SIZE, NULL);

    // Envia arquivo
    memset(message, 0, BUFFER_SIZE);
    remain_data = atoi(filesize);
    int j = 0;
    while (remain_data > 0 && (data_read = fread(&message, 1, BUFFER_SIZE, filefd)) > 0)
    {
        int data_sent = enviar(sockfd, message, data_read, NULL);
        remain_data -= data_sent;
        memset(message, 0, BUFFER_SIZE);
        j++;
        printf("  enviando[%d]: fread()::%d send()::%d remaindata::%d filesize::%s\n", j, data_read, data_sent, remain_data, filesize);
    }

    // Limpa a sujeira
    fclose(filefd);
    free(filepath);
}

int process_username(SSL *sockfd, const char* buffer, int id)
{
    char message[BUFFER_SIZE];
    struct stat st = {0};
    char* copy;
    char* command;
    char* username;

    // Pega dados do comando
    copy = strdup(buffer);
    command = strdup(strtok(copy, " "));
    username =  strdup(strtok(NULL, " "));

    char* syncpath;
    struct passwd* pw = getpwuid(getuid());
    char* homedir = strdup(pw->pw_dir);
    syncpath = malloc(BUFFER_SIZE);
    memset(syncpath, 0, BUFFER_SIZE);
    strcpy(syncpath, homedir);

    // Monta folderpath
    if (strcmp(username, "rmserver") == 0)
    {
        pthread_mutex_lock(&lock);
        strcpy(client_info_array[id].username, username);
        memset(client_info_array[id].folderPath, 0, PATH_SIZE);
        strcpy(client_info_array[id].folderPath, syncpath);
        strcat(client_info_array[id].folderPath, "/server/");
        char* folder = strdup(client_info_array[id].folderPath);
        pthread_mutex_unlock(&lock);

        memset(message, 0, BUFFER_SIZE);
        strcpy(message, "rmok");
        enviar(sockfd, message, BUFFER_SIZE, NULL);
    } else {
        pthread_mutex_lock(&lock);
        strcpy(client_info_array[id].username, username);
        memset(client_info_array[id].folderPath, 0, PATH_SIZE);
        strcpy(client_info_array[id].folderPath, syncpath);
        strcat(client_info_array[id].folderPath, "/server/");
        strcat(client_info_array[id].folderPath, username);
        strcat(client_info_array[id].folderPath, "/");
        char* folder = strdup(client_info_array[id].folderPath);
        pthread_mutex_unlock(&lock);

        // Acha um client_id valido
        int i = 0;
        int connections = 0;
        pthread_mutex_lock(&lock);
        for (i = 0; i < NUM_MAX_CLIENT; ++i)
        {
            int cmp = strcmp(client_info_array[i].username, username);
            if (cmp == 0)
            {
                ++connections;
            }
        }

        if (connections > 2)
        {
            memset(message, 0, BUFFER_SIZE);
            strcpy(message, "NOTOK");
            closeClient(id);
            enviar(sockfd, message, BUFFER_SIZE, NULL);

            return;
        }
        pthread_mutex_unlock(&lock);

        int client_id = findSlotId();
        if(client_id == -1)
            {
            printf("[server] i am full!\n");
            memset(message, 0, BUFFER_SIZE);
            strcpy(message, "SERVER");
            closeClient(id);
            enviar(sockfd, message, BUFFER_SIZE, NULL);
            return;
        }

        // Cria pasta do usuario
        if (stat("folder", &st) == -1) {
            mkdir(folder, 0700);
        }

        // Envia ok
        memset(message, 0, BUFFER_SIZE);
        strcpy(message, "OK");
        strcat(message, " ");
        strcat(message, rm_ip);
        strcat(message, " ");
        strcat(message, port);
        strcat(message, " ");
        enviar(sockfd, message, BUFFER_SIZE, NULL);
        printf("3\n");
    }
}

int process_hi(SSL *sockfd, const char* buffer, int id)
{
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "HI OK");
    enviar(sockfd, message, BUFFER_SIZE, NULL);
}

int process_gettime(SSL *sockfd, const char* buffer, int id)
{
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);

    time_t servertime;
    time(&servertime);
    sprintf(servertime, "%d"); // Não entendi o 3º param dessa func. Me parece ser opcional.

    strcpy(message, servertime);
    enviar(sockfd, message, BUFFER_SIZE, NULL);
}

int process_list(SSL *sockfd, const char* buffer, int id)
{
    char pString[BUFFER_SIZE];
    char* folderpath;
    DIR* pDir;
    struct dirent* pDirent;

    pthread_mutex_lock(&lock);
    folderpath = strdup(client_info_array[id].folderPath);
    pthread_mutex_unlock(&lock);

    memset(pString, 0, BUFFER_SIZE);
    strcpy(pString, "[server] files: \n/");

    pDir = opendir(folderpath);

    if (pDir == NULL) {
        memset(pString, 0, BUFFER_SIZE);
        strcpy(pString, "ERROR DIR NOT FOUND");
        enviar(sockfd, pString, BUFFER_SIZE, NULL);
        return -1;
    }

    while ((pDirent = readdir(pDir)) != NULL) {
        if(pDirent->d_type == DT_REG)
        {
            char* name = pDirent->d_name;
            strcat(pString, name);
            strcat(pString, "\n/");
        }
    }

    closedir(pDir);

    enviar(sockfd, pString, BUFFER_SIZE, NULL);

    free(folderpath);
}

int process_error(SSL *sockfd, const char* buffer, int id)
{
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "ERROR COMMAND NOT FOUND");
    enviar(sockfd, message, BUFFER_SIZE, NULL);
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

int receive_one_file(SSL *sockfd, char* filepath, char* filename, int filesize)
{
    printf("receive_one_file hi\n");

    FILE* filefd;
    int remain_data = filesize;
    int data_read = 0;
    char message[BUFFER_SIZE];

    printf("receive_one_file filepath::%s\n", filepath);

    filefd = fopen(filepath, "wb");

    if (filefd == NULL)
    {
        printf("fopen error\n");
        return -1;
    }

    printf("fopen ok\n");

    memset(message, 0, BUFFER_SIZE);

    int j =0;

    while (remain_data > 0 && (data_read = receber(sockfd, message, BUFFER_SIZE, NULL)) > 0)
    {
        int data_write = fwrite(&message, 1, data_read, filefd);

        remain_data -= data_read;

        memset(message, 0, BUFFER_SIZE);

        j++;

        printf("  recv[%d]: dataread::%d datawrite::%d remaindata::%d filesize::%d j::%d \n", j, data_read, data_write, remain_data, filesize);
    }

    fclose(filefd);

    printf("transfer done\n");

    // clean
    //receber(sockfd, message, BUFFER_SIZE, MSG_WAITALL);
}

int process_sync_server(SSL *sockfd, const char* buffer, int id)
{
    int ret = 0;
    // printf("OLHA AQUI OWWWWWWW: %s\n", buffer);
    // Pega folderpath
    char* folderpath;
    char* username;
    pthread_mutex_lock(&lock);
    folderpath = strdup(client_info_array[id].folderPath);
    username = strdup(client_info_array[id].username);
    client_info_array[id].new_files = 1;
    pthread_mutex_unlock(&lock);

    if (strcmp(username, "rmserver") == 0)
    {
        char* copy = strdup(buffer);
        username = strdup(strtok(copy, " "));
        username = strdup(strtok(NULL, " "));
        username = strdup(strtok(NULL, " "));
        strcat(folderpath,"/");
        strcat(folderpath,username);
        strcat(folderpath,"/");
    }
    // Apaga tudo
    deleteAllFiles(folderpath);

    // Recebe tudo
    while(1)
    {
        char message[BUFFER_SIZE];
        memset(message, 0, BUFFER_SIZE);
        strcpy(message, "NEXT");
        strcat(message," ");
        strcat(message,"arg1");
        ret = enviar(sockfd, message, BUFFER_SIZE, NULL);

        memset(message, 0, BUFFER_SIZE);
        ret = receber(sockfd, message, BUFFER_SIZE, MSG_WAITALL);

        char* arg1 = strdup(strtok(message, " "));


        if(strcmp(arg1, "FILE") == 0)
        {
            char* filename = strdup(strtok(NULL, " "));
            char* filesize = strdup(strtok(NULL, " "));

            char filepath[1024];
            strcpy(filepath, folderpath);
            strcat(filepath, filename);

            // printf("process_sync_client: filepath::[%s]\n", filepath);
            // printf("process_sync_client: filename::[%s]\n", filename);
            // printf("process_sync_client: filesize::[%s]\n", filesize);

            receive_one_file(sockfd, filepath, filename, atoi(filesize));
            free(filename);
            free(filesize);
        }
        else if(strcmp(arg1, "FIM") == 0)
        {
            break;
        }
    }
    printf("chamando RMSYNC\n");
    process_rmsync(sockfd, buffer, id);
    printf("retornou RMSYNC\n");
    printf("server sync FINALIZADO\n");
}

int createAndListen(char* ip, int port)
{
    // Server socket
    struct sockaddr_in sock_info;
    int sock_number;
    int bind_number;
    int listen_number;

    // Reseta estrutura que hold data dos clientes
    pthread_mutex_lock(&lock);
    int i;
    for(i=0; i<NUM_MAX_CLIENT; i++)
    {
        client_info_array[i].client_id = -1;
        client_info_array[i].isActive = -1;
        memset(client_info_array[i].folderPath, 0, PATH_SIZE);
        memset(client_info_array[i].username, 0, USERNAME_SIZE);
    }
    pthread_mutex_unlock(&lock);

    // Fill sockaddr_in structure
    sock_info.sin_family = AF_INET;
    sock_info.sin_addr.s_addr = inet_addr(ip);
    sock_info.sin_port = htons(port);

    // Create socket
    sock_number = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_number == -1)
    {
        printf("[server] socket failed (errstr=%s) (errno=%d)\n", strerror(errno), errno);
        return -1;
    }

    // Bind
    bind_number = bind(sock_number, (struct sockaddr*) &sock_info, sizeof(sock_info));
    if (bind_number < 0)
    {
        printf("[server] bind failed (errstr=%s) (errno=%d)\n", strerror(errno), errno);
        return -1;
    }

    // Listen
    listen_number = listen(sock_number , 5);
    if(listen_number < 0)
    {
        printf("[server] listen failed (errstr=%s) (errno=%d)\n", strerror(errno), errno);
        return -1;
    }

    printf("[server] server is up, listening on %s:%d\n", inet_ntoa(sock_info.sin_addr), ntohs(sock_info.sin_port));
    return sock_number;
}

int closeClient(int client_id)
{
    // Limpa struct
    pthread_mutex_lock(&lock);
    client_info_array[client_id].client_id = -1;
    memset(client_info_array[client_id].folderPath, 0, PATH_SIZE);
    memset(client_info_array[client_id].username, 0, USERNAME_SIZE);
    client_info_array[client_id].isActive = -1;
    pthread_mutex_unlock(&lock);
}

int findSlotId()
{
    int client_id = -1;

    pthread_mutex_lock(&lock);
    int i;
    for(i=0; i<NUM_MAX_CLIENT; i++)
    {
        printf("[server] slot [%d] = %d\n",i,client_info_array[i].isActive);
    }
    for(i=0; i<NUM_MAX_CLIENT; i++)
    {
        if(client_info_array[i].isActive == -1)
        {
            client_info_array[i].isActive = 0;
            client_id = i;
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    return client_id;
}

int process_rmsync(SSL* client_number, const char* buffer, int client_id)
{
    printf("rmsync ON\n");
    if (rmssl == NULL)
    {
        printf("rmssl NULL, volta\n");
        return 0;
    }

    DIR* pDir;
    struct dirent* pDirent;
    int ret;

    pthread_mutex_lock(&lock);
    char* user = strdup(client_info_array[client_id].username);
    pthread_mutex_unlock(&lock);
    // Get home user folder
    char folderpath[BUFFER_SIZE];
    struct passwd* pw = getpwuid(getuid());
    memset(folderpath, 0, BUFFER_SIZE);
    strcpy(folderpath, pw->pw_dir);
    strcat(folderpath, "/server/");
    strcat(folderpath, user);
    strcat(folderpath, "/");

    // Envia comando para o server
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "SYNCSERVER");
    strcat(message," ");
    strcat(message,"arg1");
    strcat(message," ");
    strcat(message,user);
    strcat(message," ");
    ret = enviar(rmssl, message, BUFFER_SIZE, NULL);
    if(ret < 0)
    {
        printf("[RM] process_rmsync: send error\n", NULL);
    }


    // Envia arquivos
    pDir = opendir(folderpath);
    if (pDir == NULL) {
        printf("process_rmsync: opendir error :(\n", NULL);
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
            receber(rmssl, message, BUFFER_SIZE, MSG_WAITALL);


            // Envia um
            filefd = fopen (filepath, "rb");
            if (filefd == NULL) {
                printf("process_rmsync: fopen error :(\n", NULL);
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
            ret = enviar(rmssl, message, BUFFER_SIZE, NULL);
            if(ret < 0)
            {
                printf("process_rmsync: send error :(\n", NULL);
            }


            // Envia arquivo
            memset(message, 0, BUFFER_SIZE);
            remain_data = atoi(filesizes);
            int j =0;
            while (remain_data > 0 && (data_read = fread(&message, 1, BUFFER_SIZE, filefd)) > 0)
            {
                j++;

                int data_sent = enviar(rmssl, message, data_read, NULL);

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
    receber(rmssl, message, BUFFER_SIZE, MSG_WAITALL);

    // Avisa que terminou
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "FIM A B C D");
    enviar(rmssl, message, BUFFER_SIZE, NULL);
    printf("rmsync OFF\n");
}

int process_sync_client(SSL *sockfd, const char* buffer, int id)
{
    // printf("process_sync_client(): iniciando...\n");

    char message[BUFFER_SIZE];
    int ret;
    DIR* pDir;
    struct dirent* pDirent;


    // Pasta do usuario
    char* folderpath;
    pthread_mutex_lock(&lock);
    folderpath = strdup(client_info_array[id].folderPath);
    pthread_mutex_unlock(&lock);
    // printf("process_sync_client(): pasta do usuario::%s\n", folderpath);


    // Envia todos arquivos
    pDir = opendir(folderpath);
    if (pDir == NULL) {
        printf("process_sync_client: opendir error :(\n");
        return -1;
    }
    while ((pDirent = readdir(pDir)) != NULL) {
        if(pDirent->d_type == DT_REG)
        {
            FILE* filefd;
            int data_read = 0;
            int remain_data = 0;
            char filepath[1024];
            char filename[512];
            char filesizes[512];
            int filesize;
            char message[BUFFER_SIZE];


            // Recebe pedido de um
            // printf("Waiting client permission...\n");
            memset(buffer, 0, BUFFER_SIZE);
            receber(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL);
            // printf("process_sync_client(): recebido::%s\n", buffer);


            // Filepath
            strcpy(filename, pDirent->d_name);
            strcpy(filepath, folderpath);
            strcat(filepath, "/");
            strcat(filepath, filename);
            // printf("process_sync_client(): filepath::%s\n", filepath);

            // Abre arquivo
            filefd = fopen (filepath, "rb");
            if (filefd == NULL) {
                printf("process_sync_client: fopen error :(\n");
                return -1;
            }
            // printf("process_sync_client(): arquivo aberto\n");


            // Pega filesize
            fseek(filefd, 0L, SEEK_END);
            sprintf(filesizes, "%ld", ftell(filefd));
            rewind(filefd);
            remain_data = atoi(filesizes);
            filesize = atoi(filesizes);
            // printf("process_sync_client(): filesize::%d\n",filesize);


            // Envia FILE
            memset(message, 0, BUFFER_SIZE);
            strcpy(message, "FILE");
            strcat(message," ");
            strcat(message, filename);
            strcat(message," ");
            strcat(message, filesizes);
            ret = enviar(sockfd, message, BUFFER_SIZE, NULL);
            if(ret < 0)
            {
                printf("process_sync_client: send error :(\n");
            }
            // printf("process_sync_client(): enviado::%s\n",message);


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

            // printf("process_sync_client(): FINALIZADO\n");
            fclose(filefd);
        }
    }

    closedir(pDir);

    // Espera client pedir mais um
    // printf("Waiting client permission...\n");
    memset(buffer, 0, BUFFER_SIZE);
    receber(sockfd, buffer, BUFFER_SIZE, MSG_WAITALL);
    // printf("process_sync_client(): recebido::%s\n", buffer);

    // Avisa que terminou
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "FIM A B C D");
    enviar(sockfd, message, BUFFER_SIZE, NULL);
    // printf("process_sync_client(): enviado FIM::%s\n", message);


    free(folderpath);
}

int process_newfiles(SSL *sockfd, const char* buffer, int id)
{
    printf("\n process_newfiles(): Hi \n");

    char* copy = strdup(buffer);
    printf("1 process_newfiles(): buffer: %s\n", copy);
    char* firstString = strtok(copy, " ");
    char* username = strtok(NULL, " ");
    printf("2 process_newfiles(): nome do usuario %s\n", username);

    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);

    int cmp;
    int nf;
    int cid;
    char* user;
    int i = 0;

    // Procura pelo usuario
    for (i = 0; i < NUM_MAX_CLIENT; ++i)
    {

        pthread_mutex_lock(&lock);
        nf = client_info_array[i].new_files;
        cid = client_info_array[i].client_id;
        user = strdup(client_info_array[i].username);
        pthread_mutex_unlock(&lock);

        // Avisa para sincronizar NOTOK
        cmp = strcmp(user, username);
        if (cmp == 0 && cid != id && nf == 1)
        {
            pthread_mutex_lock(&lock);
            client_info_array[i].new_files = 0;
            pthread_mutex_unlock(&lock);
            printf("3 process_newfiles(): VAMO SYNC\n");
            strcpy(message, "NOTOK");
            enviar(sockfd, message, BUFFER_SIZE, NULL);
            return 0;
        }
    }

    // Avisa para nao sincronizar OK
    // printf("4 process_newfiles(): nao vamo sync\n");
    strcpy(message, "OK");
    enviar(sockfd, message, BUFFER_SIZE, NULL);

    printf("\n process_newfiles(): Bye \n");

    return 0;
}

void* rm_function(void* thread_function_arg)
{
    struct sockaddr_in serveraddr;
    char buffer[BUFFER_SIZE];
    int sockfd;

    printf("[RM] enter server ip: ");
    scanf("%s", rm_ip);

    printf("[RM] enter server port: ");
    scanf("%s", port);
    rm_port = atoi(port);

    memset(buffer, 0, BUFFER_SIZE);

    // Cria
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd < 0)
    {
        printf("[RM] error opening socket\n", NULL);
        return 1;
    }

    // Configura
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(rm_ip);
    serveraddr.sin_port = htons(rm_port);

    // Conecta
    if (connect(sockfd , (struct sockaddr *) &serveraddr , sizeof(serveraddr)) < 0)
    {
        printf("[RM] error connecting to server\n", NULL);
        return 1;
    }

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

    rmssl = SSL_new(ctx);
    SSL_set_fd(rmssl, sockfd);
    if (SSL_connect(rmssl) == -1)
        ERR_print_errors_fp(stderr);
    else {
        //GG
        X509 *cert;
        char *line;
        cert = SSL_get_peer_certificate(rmssl);
        if (cert != NULL) {
            printf("[RM] Client certificates:\n");
            line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
            printf("[RM] Subject: %s\n", line);
            free(line);
            line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
            printf("[RM] Issuer: %s\n", line);
            free(line);
            X509_free(cert);
        }
        else
            printf("[RM] No certificates.\n");
    }

    // Envia HI
    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "HI");
    if(enviar(rmssl, buffer, BUFFER_SIZE, 0)  < 0)
    {
        printf("[client] RM Hi sent failed\n", NULL);
        return 1;
    }

    // Recebe HI
    memset(buffer, 0, BUFFER_SIZE);
    if(receber(rmssl, buffer, BUFFER_SIZE, MSG_WAITALL) < 0)
    {
        printf("[client] RM HI recv failed\n", NULL);
        return 1;
    }

    // Envia username
    memset(buffer, 0, BUFFER_SIZE);
    strcpy(buffer, "USERNAME");
    strcat(buffer, " ");
    strcat(buffer, "rmserver");
    strcat(buffer, "\0");
    if (enviar(rmssl, buffer, BUFFER_SIZE, 0) < 0)
    {
        puts("[RM] send failed.\n");
        return 1;
    }

    // Recebe RM
    memset(buffer, 0, BUFFER_SIZE);
    if( receber(rmssl, buffer, BUFFER_SIZE, MSG_WAITALL) < 0)
    {
        printf("[client] RM recv failed.\n", NULL);
        return 1;
    }

    char* copy;
    char* command;

    // Pega dados do comando
    copy = strdup(buffer);
    command = strdup(strtok(copy, " "));

    if (strcmp(command, "rmok") != 0)
    {
        printf("[RM] failed to connect to primary RM\n");
        exit(0);
    }
    printf("[RM] connected to primary server\n", NULL);

    free(command);
}

void* thread_function(void* thread_function_arg)
{
    // Client
    int client_id;
    struct sockaddr_in client_addr;
    int client_number;
    SSL *client_ssl;

    // Communication
    char buffer[BUFFER_SIZE];
    int read_size;

    // From thread
    client_id = (int) thread_function_arg;


    // Pega dados da shared
    pthread_mutex_lock(&lock);
    client_addr = client_info_array[client_id].client_info;
    client_number = client_info_array[client_id].client_number;
    client_ssl = client_info_array[client_id].ssl;
    pthread_mutex_unlock(&lock);


    // Recebe comando do cliente
    memset(buffer, 0, BUFFER_SIZE);

    while ((read_size = receber(client_ssl, buffer, BUFFER_SIZE, MSG_WAITALL)) > 0)
    {

        printf("\n\n CLIENT COMMAND:[%s]\n\n", buffer);

        switch(getCommand(buffer))
        {
            case CMD_HI:
                process_hi(client_ssl, buffer, client_id);
            break;

            case CMD_USERNAME:
                process_username(client_ssl, buffer, client_id);
            break;

            case CMD_LIST:
                process_list(client_ssl, buffer, client_id);
            break;

            case CMD_RECV:
                process_recv(client_ssl, buffer, client_id);
            break;

            case CMD_SEND:
                process_send(client_ssl, buffer, client_id);
            break;

            case CMD_SYNC_CLIENT:
                process_sync_client(client_ssl, buffer, client_id);
            break;

            case CMD_SYNC_SERVER:
                process_sync_server(client_ssl, buffer, client_id);
            break;

            case CMD_NEWFILES:
                process_newfiles(client_ssl, buffer, client_id);
            break;

            case CMD_GETTIME:
                process_gettime(client_number, buffer, client_id);
            break;

            case CMD_RMSYNC:
                process_rmsync(client_number, buffer, client_id);
            break;

            default:
                process_error(client_ssl, buffer, client_id);
            break;
        }

        memset(buffer, 0, BUFFER_SIZE);
    }

    // Check
    if(read_size == 0)
    {
        puts("[server] client disconnected.\n");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("[server] recv failed.\n");
    }

    closeClient(client_id);

    return 0;
}

int main()
{
    // Threads
    pthread_t mypthreads[NUM_MAX_CLIENT];
    pthread_t rm_thread;
    // Server
    int server_number;

    char port[IP_SIZE];

    SSL_METHOD *method;
    SSL_CTX *ctx;

    printf("[server] enter server ip: ");
    scanf("%s", server_ip);

    printf("[server] enter server port: ");
    scanf("%s", port);
    server_port = atoi(port);

    //init CTX
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }

    //load certificates
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, "CertFile.pem", SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, "KeyFile.pem", SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }

    // Create, Bind, Listen
    server_number = createAndListen(server_ip, server_port);

    if(server_number < 0)
    {
        printf("[server] server failed em iniciar\n");
    }

    char* syncpath;
    struct passwd* pw = getpwuid(getuid());
    char* homedir = strdup(pw->pw_dir);
    syncpath = malloc(BUFFER_SIZE);
    memset(syncpath, 0, BUFFER_SIZE);
    strcpy(syncpath, homedir);
    strcat(syncpath, "/server/");
    mkdir(syncpath, 0700);

    //server as client ~ replica manager
    char answer[USERNAME_SIZE];
    rmssl = NULL;
    printf("[server] connect to primary RM? (y/n)\n");
    scanf("%s", answer);
    if (strcmp(answer, "y") == 0)
    {
        int threadsv = pthread_create(&rm_thread, NULL, rm_function, NULL);
        if (threadsv < 0)
        {
            printf("[server] RM pthread_create failed (errstr=%s) (errno=%d)\n", strerror(errno), errno);
            exit(0);
        }
    }

    // Aceita clientes para sempre
    int client_id = 0;
    for (;;)
    {
        // Client
        struct sockaddr_in client_info;
        int client_info_size;
        int client_number;

        // Thread
        int threadfd;


        // Accept client
        printf("[server] waiting for client...\n");

        client_info_size = sizeof(struct sockaddr_in);
        client_number = accept(server_number, (struct sockaddr*) &client_info, (socklen_t*) &client_info_size);
        if (client_number < 0)
        {
            printf("[server] accept failed (errstr=%s) (errno=%d)\n", strerror(errno), errno);
            return -1;
        }


        // Seta client info
        pthread_mutex_lock(&lock);
        client_info_array[client_id].client_id = client_id;
        client_info_array[client_id].client_number = client_number;
        client_info_array[client_id].client_info = client_info;
        // create and set SSL context
        client_info_array[client_id].ssl = SSL_new(ctx);
        SSL_set_fd(client_info_array[client_id].ssl, client_number);
        if ( SSL_accept(client_info_array[client_id].ssl) == -1 )
            ERR_print_errors_fp(stderr);
        pthread_mutex_unlock(&lock);
        // else
        // {
        //     X509 *cert;
        //     char *line;

        //     cert = SSL_get_peer_certificate(ssl);
        //     if ( cert != NULL )
        //     {
        //         printf("[server] Server certificates:\n");
        //         line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        //         printf("[server] Subject: %s\n", line);
        //         free(line);
        //         line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        //         printf("[server] Issuer: %s\n", line);
        //         free(line);
        //         X509_free(cert);
        //     }
        //     else
        //         printf("[server] No certificates.\n");
        // }

        // Inicia thread
        threadfd = pthread_create(&mypthreads[client_id], NULL, thread_function, (void*) client_id);
        if (threadfd < 0)
        {
            printf("[server] pthread_create failed (errstr=%s) (errno=%d)\n", strerror(errno), errno);
            return -1;
        }

        //
        client_id++;

        // Respira kkk
        sleep(10);
    }

    // Espera threads
    /*
    for (client_id = 0; client_id < NUM_MAX_CLIENT; client_id++)
    {
        pthread_join(&mypthreads[client_id]);
    }
    */

    // Safety
    pthread_exit(0);

    return 0;
}
