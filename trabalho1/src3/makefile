all: server1 client1

clean:
	rm *.o

cleanall:
	rm *.o CLIENT SERVER

server1: dropboxServer.o dropboxUtils.o
	gcc -pthread -o SERVER dropboxServer.o dropboxUtils.o

client1: dropboxClient.o dropboxUtils.o
	gcc -o CLIENT dropboxClient.o dropboxUtils.o

dropboxServer.o: dropboxServer.c
	gcc -pthread -c dropboxServer.c

dropboxClient.o: dropboxClient.c
	gcc -c dropboxClient.c

dropboxUtils.o: dropboxUtils.c
	gcc -c dropboxUtils.c

