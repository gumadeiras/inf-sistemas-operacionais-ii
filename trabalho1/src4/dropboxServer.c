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

// Server config
#define LISTEN_IP "127.0.0.1"
#define LISTEN_PORT 3002
#define ROOT_PATH "/home/mauricio/Desktop/server/"
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

struct client_info
{
    int client_id;
    int client_number;
    struct sockaddr_in client_info;
    char folderPath[PATH_SIZE];
    char isActive;
    char username[USERNAME_SIZE];
};
struct client_info client_info_array[NUM_MAX_CLIENT]; // SHARED VARIABLE ONE
pthread_mutex_t lock; // SHARED-VARIABLE-ONE LOCK

char toLowercase(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? (ch + 32) : (ch);
}

void turnStringLowercase(char* string)
{
    for (int index=0; string[index]; index++)
    {
        string[index] = toLowercase(string[index]);
    }
}

int getCommand(const char* string)
{
    char* copy = strdup(string);
    char* firstString = strtok(copy, " ");

    int command = CMD_ERROR;

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

    free(copy);

    return command;
}

 // void receive_file(char *file);
int process_recv(const int sockfd, const char* buffer, int id)
{
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
    remain_data = atoi(filesize);
    while (remain_data > 0 && (data_read = recv(sockfd, message, BUFFER_SIZE, NULL)) > 0)
    {
        fwrite(&message, 1, data_read, filefp);
        remain_data -= data_read;
        printf("[server] File: [%d/%d - %d]\n", remain_data, atoi(filesize), data_read);
    }
    printf("[server] Transfer completed\n");

    // Limpa a sujeira
    fclose(filefp);
    free(filepath);
    free(copy);
}

// void receive_file(char *file);
int process_send(const int sockfd, const char* buffer, int id)
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
        write(sockfd, message, BUFFER_SIZE);
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
    send(sockfd, message, BUFFER_SIZE, NULL);

    // Envia arquivo
    memset(message, 0, BUFFER_SIZE);
    remain_data = atoi(filesize);
    while (remain_data > 0 && (data_read = fread(&message, 1, BUFFER_SIZE, filefd)) > 0)
    {
        int data_sent = send(sockfd, message, data_read, NULL);
        remain_data -= data_sent;
        memset(message, 0, BUFFER_SIZE);
    }

    // Limpa a sujeira
    fclose(filefd);
    free(filepath);
}

int process_username(const int sockfd, const char* buffer, int id)
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

    // Faz login

    // Monta folderpath
    pthread_mutex_lock(&lock);
    memset(client_info_array[id].folderPath, 0, PATH_SIZE);
    strcpy(client_info_array[id].folderPath, ROOT_PATH);
    strcat(client_info_array[id].folderPath, "/");
    strcat(client_info_array[id].folderPath, username);
    strcat(client_info_array[id].folderPath, "/");
    char* folder = strdup(client_info_array[id].folderPath);
    pthread_mutex_unlock(&lock);

    // Cria pasta do usuario
    if (stat("folder", &st) == -1) {
        mkdir(folder, 0700);
    }

    // Envia ok
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "USERNAME OK");
    write(sockfd, message, BUFFER_SIZE);

    // Limpa sujeira
    free(folder);
}

int process_hi(const int sockfd, const char* buffer, int id)
{
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "HI OK");
    send(sockfd, message, BUFFER_SIZE, NULL);
}

int process_list(const int sockfd, const char* buffer, int id)
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
        write(sockfd, pString, BUFFER_SIZE);
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

    write(sockfd, pString, BUFFER_SIZE);

    free(folderpath);
}

int process_error(const int sockfd, const char* buffer, int id)
{
    char message[BUFFER_SIZE];
    memset(message, 0, BUFFER_SIZE);
    strcpy(message, "ERROR COMMAND NOT FOUND");
    write(sockfd, message, BUFFER_SIZE);
}

void* thread_function(void* thread_function_arg)
{
    // Client
    int client_id;
    struct sockaddr_in client_addr;
    int client_number;

    // Communication
    char buffer[BUFFER_SIZE];
    int read_size;

    // From thread
    client_id = (int) thread_function_arg;

    // Pega dados da shared
    pthread_mutex_lock(&lock);
    client_addr = client_info_array[client_id].client_info;
    client_number = client_info_array[client_id].client_number;
    pthread_mutex_unlock(&lock);

    // Recebe comando do cliente
    memset(buffer, 0, BUFFER_SIZE);

    while ((read_size = recv(client_number, buffer, BUFFER_SIZE, MSG_WAITALL)) > 0)
    {
        switch(getCommand(buffer))
        {
            case CMD_HI:
                process_hi(client_number, buffer, client_id);
            break;

            case CMD_USERNAME:
                process_username(client_number, buffer, client_id);
            break;

            case CMD_LIST:
                process_list(client_number, buffer, client_id);
            break;

            case CMD_RECV:
                process_recv(client_number, buffer, client_id);
            break;

            case CMD_SEND:
                process_send(client_number, buffer, client_id);
            break;

            default:
                process_error(client_number, buffer, client_id);
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

int createAndListen(char* ip, int port)
{
	// Server socket
    struct sockaddr_in sock_info;
    int sock_number;
    int bind_number;
    int listen_number;

    // Reseta estrutura que hold data dos clientes
    pthread_mutex_lock(&lock);
    for(int i=0; i<NUM_MAX_CLIENT; i++)
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
	for(int i=0; i<NUM_MAX_CLIENT; i++)
	{
		printf("[server] slot [%d] = %d\n",i,client_info_array[i].isActive);
	}
	for(int i=0; i<NUM_MAX_CLIENT; i++)
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

int main()
{
    // Threads
    pthread_t mypthreads[NUM_MAX_CLIENT];

    // Server
    int server_number;

    // Create, Bind, Listen
    server_number = createAndListen(LISTEN_IP, LISTEN_PORT);
    if(server_number < 0)
    {
    	printf("[server] server failed em iniciar\n");
    }
    printf("[server] server iniciado com sucesso! ip=%s port=%d\n", LISTEN_IP, LISTEN_PORT);

    // Aceita clientes para sempre
    for (;;)
    {
    	// Server
    	int total;

    	// Client
	    struct sockaddr_in client_info;
	    int client_info_size;
	    int client_number;
	    int client_id;
	    
	    // Thread
	    int threadfd;

    	// Acha um client_id valido
    	client_id = findSlotId();
    	if(client_id == -1)
    	{
    		printf("[server] i am full!\n");
    		sleep(100);
    		continue;
    	}
    	
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
        pthread_mutex_unlock(&lock);

        // Inicia thread
        threadfd = pthread_create(&mypthreads[client_id], NULL, thread_function, (void*) client_id);
        if (threadfd < 0)
        {
            printf("[server] pthread_create failed (errstr=%s) (errno=%d)\n", strerror(errno), errno);
            return -1;
        }

        // Respira
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
