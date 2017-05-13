//*
//* Trabalho 1
//* Sistemas Operacionais II N - 2017/1
//* Prof. Dr. Alberto Egon Schaeffer Filho
//*
//* dropboxServer.c
//* implementação das funções do servidor
//*
//* chamada: ./dropboxClient fulano endereço porta
//*

struct client
{
  int devices[2]; // associado aos dispositivos do usuário userid[MAXNAME]
  char userid[MAXNAME]; // id do usuário no servidor, que deverá ser único. Informado pela linha de comando.
  struct file_info[MAXFILES]; // metadados de cada arquivo que o cliente possui no servidor.
  int logged_in; // cliente está logado ou não.
};

struct file_info
{
  char name[MAXNAME]; // refere-se ao nome do arquivo. extension[MAXNAME]
  char extension[MAXNAME]; // refere-se ao tipo de extensão do arquivo.
  char last_modified[MAXNAME]; // refere-se a data da última mofidicação no arquivo.
  int size; // indica o tamanho do arquivo, em bytes.
};



//* Sincroniza o servidor com o diretório “sync_dir_<nomeusuário>” com o cliente.
void sync_server();

//* Recebe um arquivo file do cliente.
//* Deverá ser executada quando for realizar upload de um arquivo. file – path/filename.ext do arquivo a ser recebido
void receive_file(char *file);

//* Envia o arquivo file para o usuário.
//* Deverá ser executada quando for realizar download de um arquivo. file – filename.ext
void send_file(char *file);