//*
//* Trabalho 1
//* Instituto de Informática - UFRGS
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxServer.h
//* header das funções do servidor
//*

void sync_server();
void receive_file(char *file);
void send_file(char *file);
int server_init(char *s_ip, char *s_port);
int verify_user_server();