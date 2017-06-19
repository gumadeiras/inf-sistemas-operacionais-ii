//*
//* Trabalho 1
//* Instituto de Informática - UFRGS
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxServer.h
//* implementação das funções do cliente
//*


 // void receive_file(char *file);
int process_recv(const int sockfd, const char* buffer, int id);

// void receive_file(char *file);
int process_send(const int sockfd, const char* buffer, int id);
char toLowercase(char ch);
void turnStringLowercase(char* string);
int getCommand(const char* string);
int process_username(const int sockfd, const char* buffer, int id);
int process_hi(const int sockfd, const char* buffer, int id);
int process_list(const int sockfd, const char* buffer, int id);
int process_error(const int sockfd, const char* buffer, int id);
void* thread_function(void* thread_function_arg);
int createAndListen(char* ip, int port);
int closeClient(int client_id);
int findSlotId();