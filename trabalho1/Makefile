#
# Trabalho 1
# Instituto de Informática - UFRGS
# Sistemas Operacionais II N - 2017/1
# Prof. Dr. Alberto Egon Schaeffer Filho
#

SRC=./src/

DIRS=$(SRC)

CC=gcc
CCFLAGS=-c -w -lm -DEBUG

all: main

main: dropboxServer dropboxClient

dropboxServer: dropboxServer.o
	$(CC) dropboxServer.o -o dropboxServer -lpthread

dropboxClient: dropboxClient.o
	$(CC) dropboxClient.o -o dropboxClient -lpthread

dropboxServer.o:
	$(CC) $(CCFLAGS) $(SRC)dropboxServer.c

dropboxClient.o:
	$(CC) $(CCFLAGS) $(SRC)dropboxClient.c

# dropboxServer.o: dropboxUtil.o
# 	$(CC) $(CCFLAGS) dropboxServer.o dropboxUtil.o -o dropboxServer

# dropboxClient.o: dropboxUtil.o
# 	$(CC) $(CCFLAGS) dropboxClient.o dropboxUtil.o -o dropboxClient

# dropboxUtil.o:
# 	$(CC) $(CCFLAGS) dropboxUtil.o

clean:
	find . -type f ! -name "*.c" ! -name "*.h" ! -name "Makefile" ! -name "*.pdf" ! -name "*.tex" ! -name "*.png" -delete
