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

#define ROOT_PATH "/home/gustavo/Desktop/server/"

#define NUM_MAX_CLIENT 10
#define LISTEN_PORT 3002

#define BUFFER_SIZE 1024
#define USERNAME_SIZE 100
#define PATH_SIZE 2048

#define CMD_ERROR -1
#define CMD_HI 1
#define CMD_USERNAME 2
#define CMD_LIST 3
#define CMD_RECV 4

struct client_info
{
    int client_id;
    struct sockaddr_in client_addr;
    int client_sockfd;
    int client_len;
    char folderPath[PATH_SIZE];
    char isActive;
    char username[USERNAME_SIZE]
};

struct client_info client_info_array[NUM_MAX_CLIENT]; // SHARED VARIABLE

pthread_mutex_t lock;

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
    int command = CMD_ERROR;

    char* copy = strdup(string);

    char* firstString = strtok(copy, " ");

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

    free(copy);

    return command;
}

char* getListString(int id, char* path)
{
    DIR* pDir;
    struct dirent* pDirent;

    char* pString = malloc(sizeof(char) * BUFFER_SIZE);
    memset(pString, 0, BUFFER_SIZE);
    strcpy(pString, "LIST ");

    pDir = opendir (path);

    if (pDir == NULL) {
        printf ("Cannot open directory\n");
        strcat(pString, "ERROR");
        return pString;
    }

    while ((pDirent = readdir(pDir)) != NULL) {
        if(pDirent->d_type == DT_REG)
        {
            char* name = pDirent->d_name;
            strcat(pString, name);
            strcat(pString, " ");
        }
    }

    closedir (pDir);

    return pString;
}

void* thread_function(void* thread_function_arg)
{
    // Thread
    int client_id_arg = (int) thread_function_arg;

    // Socket
    struct sockaddr_in client_addr;
    int client_sockfd;
    int client_len;
    int read_size;
    char buffer[BUFFER_SIZE];

    // File
    char fbuffer[BUFFER_SIZE];
    FILE* filefp;
    int fread_ret;
    struct stat st = {0};

    // Inicializa
    pthread_mutex_lock(&lock);
    client_addr = client_info_array[client_id_arg].client_addr;
    client_sockfd = client_info_array[client_id_arg].client_sockfd;
    client_len = client_info_array[client_id_arg].client_len;
    pthread_mutex_unlock(&lock);

    memset(buffer, 0, BUFFER_SIZE);
    memset(fbuffer, 0, BUFFER_SIZE);

    while ((read_size = recv(client_sockfd, buffer, BUFFER_SIZE, MSG_WAITALL)) > 0)
    {
        printf("Client %d: [%s]\n", client_id_arg, buffer);

        switch(getCommand(buffer))
        {
            case CMD_HI:
                printf("CMD_HI\n");

                memset(buffer, 0, BUFFER_SIZE);
                strcpy(buffer, "HI OK");
                send(client_sockfd, buffer, strlen(buffer), NULL);
            break;

            case CMD_USERNAME:
            {
                printf("CMD_USERNAME\n");

                char* string = strdup(buffer);
                char* command = strtok(string, " ");
                char* username =  strtok(NULL, " ");

                // Seta folder
                pthread_mutex_lock(&lock);
                memset(client_info_array[client_id_arg].folderPath, 0, PATH_SIZE);
                strcpy(client_info_array[client_id_arg].folderPath, ROOT_PATH);
                strcat(client_info_array[client_id_arg].folderPath, "/");
                strcat(client_info_array[client_id_arg].folderPath, username);
                strcat(client_info_array[client_id_arg].folderPath, "/");
                char* folder = strdup(client_info_array[client_id_arg].folderPath);
                pthread_mutex_unlock(&lock);

                // Cria se pasta nao existe
                if (stat("folder", &st) == -1) {
                    mkdir(folder, 0700);
                }

                // Log
                printf("Username %s logou.\n", username);
                printf("Username folder: %s.\n", folder);

                // Envia
                memset(buffer, 0, BUFFER_SIZE);
                strcpy(buffer, "USERNAME OK");
                write(client_sockfd, buffer, strlen(buffer));

                // Limpa
                free(string);
                free(folder);
            }
            break;

            case CMD_LIST:
                printf("CMD_LIST\n");

                pthread_mutex_lock(&lock);
                char* path = strdup(client_info_array[client_id_arg].folderPath);
                pthread_mutex_unlock(&lock);

                // Lista arquivos e monte
                char* msg = getListString(client_id_arg, path); // ATOMIC

                memset(buffer, 0, BUFFER_SIZE);
                strcpy(buffer, msg);
                write(client_sockfd, buffer, strlen(buffer));

                free(msg);
                free(path);

            break;

            case CMD_RECV:
                printf("CMD_RECV\n");

                char* string = strdup(buffer);
                char* command = strtok(string, " ");
                char* filename =  strtok(NULL, " ");
                char* filesize =  strtok(NULL, " ");
                int remain_data = atoi(filesize);
                char* filepath = malloc(2048);

                memset(filepath, 0, 2048);

                pthread_mutex_lock(&lock);
                strcpy(filepath, client_info_array[client_id_arg].folderPath);
                pthread_mutex_unlock(&lock);

                strcat(filepath, "/");
                strcat(filepath, filename);

                filefp = fopen (filepath, "wb");
                if (filefp == NULL) {
                    printf("Error: %d [%s]\n", errno, strerror(errno));
                    break;
                }

                while (remain_data > 0 && (read_size = recv(client_sockfd, buffer, BUFFER_SIZE, NULL)) > 0)
                {
                    fwrite(&buffer, 1, read_size, filefp);
                    remain_data -= read_size;
                    printf("File: [%d/%d - %d]\n", remain_data, atoi(filesize), read_size);
                }

                printf("Transfer finalizada\n");
                fclose(filefp);
                free(filepath);
                free(string);

            break;

            default:
                printf("default\n");
                memset(buffer, 0, BUFFER_SIZE);
                strcpy(buffer, "COMMAND NOT FOUND");
                write(client_sockfd, buffer, strlen(buffer));
            break;
        }

        // Limpa
        memset(buffer, 0, BUFFER_SIZE);
    }

    // Check
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

    return 0;
}

int main()
{
    // Threads
    pthread_t mypthreads[NUM_MAX_CLIENT];

    // Server socket
    int server_sockfd;
    struct sockaddr_in server_addr;
    int bind_number;

    // Client socket
    struct sockaddr_in client_addr;
    int client_sockfd;
    int client_len;
    int client_id;

    // Limpa
    pthread_mutex_lock(&lock);
    for(int i=0; i<NUM_MAX_CLIENT; i++)
    {
        client_info_array[i].client_id = -1;
        client_info_array[i].isActive = -1;
        memset(client_info_array[i].folderPath, 0, PATH_SIZE);
        memset(client_info_array[i].username, 0, USERNAME_SIZE);
    }
    pthread_mutex_unlock(&lock);

    // Create socket
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd == -1)
    {
        printf("Could not create socket.");
        return -1;
    }
    printf("Socket created.");

    // Fill sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(LISTEN_PORT);

    // Bind
    bind_number = bind(server_sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (bind_number < 0)
    {
        printf("Bind failed.");
        return -1;
    }
    printf("Bind done.");

    // Listen
    listen(server_sockfd , 5);
    printf("Set to listen mode done.");

    // Aceita clientes ate explodir e finalizar
    for (client_id = 0; client_id < NUM_MAX_CLIENT; client_id++)
    {
        printf("Waiting for clients...\n");

        // Accept client
        client_len = sizeof(struct sockaddr_in);
        client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_addr, (socklen_t*) &client_len);
        if (client_sockfd < 0)
        {
            printf("Accept failed.\n");
            return -1;
        }
        printf("Client connection accepted.\n");

        // Seta client info
        pthread_mutex_lock(&lock);
        client_info_array[client_id].client_id = client_id;
        client_info_array[client_id].client_len = client_len;
        client_info_array[client_id].client_sockfd = client_sockfd;
        client_info_array[client_id].client_addr = client_addr;
        pthread_mutex_unlock(&lock);

        // Inicia thread
        int threadfd;
        threadfd = pthread_create(&mypthreads[client_id], NULL, thread_function, (void*) client_id);
        if (threadfd < 0)
        {
            printf("Could not create thread\n");
            return -1;
        }
        printf("Thread created.\n");

        // Respira
        sleep(10);
    }

    // Espera threads
    for (client_id = 0; client_id < NUM_MAX_CLIENT; client_id++)
    {
        //pthread_join(&mypthreads[client_id]);
    }

    // Safety
    pthread_exit(0);

    return 0;
}
