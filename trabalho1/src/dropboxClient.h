//*
//* Trabalho 1
//* Instituto de Informática - UFRGS
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxClient.h
//* implementação das funções do cliente
//*



// void send_file(char *file);
int process_upload(const int sockfd, const char* buffer);

// void receive_file(char *file);
int process_download(const int sockfd, const char* buffer, char* username);

char toLowercase(char ch);
void turnStringLowercase(char* string);
int get_command_from_buffer(const char* string);
int deleteAllFiles(char* folderpath);
int fill_buffer_with_command(char* buffer, int size);
int process_sync_server();
int process_sync_client();
int create_sync_inotify_home_folder();