#
# Trabalho 1
# Instituto de Informática - UFRGS
# Sistemas Operacionais II N - 2017/1
# Prof. Dr. Alberto Egon Schaeffer Filho
#

SRC=./src/

DIRS=$(SRC)

CC=gcc
CCFLAGS=-c -lm -m32 -Wall -I$(INC)

all: main

main: dropboxServer.o dropboxClient.o

dropboxServer.o: dropboxUtil.o
	$(CC) $(CCFLAGS) dropboxServer.o dropboxUtil.o -o dropboxServer

dropboxClient.o: dropboxUtil.o
	$(CC) $(CCFLAGS) dropboxClient.o dropboxUtil.o -o dropboxClient

dropboxUtil.o:
	$(CC) $(CCFLAGS) dropboxUtil.o -o

clean:
	find . -type f ! -name "*.c" ! -name "*.h" ! -name "Makefile" ! -name "*.pdf" ! -name "*.tex" ! -name "*.png" -delete
