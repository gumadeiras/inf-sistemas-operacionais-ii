all: server1 client1

clean:
	rm *.o

cleanall:
	rm *.o CLIENT SERVER

server1: dropboxServer.o dropboxUtils.o
	gcc -o SERVER dropboxServer.o dropboxUtils.o -lssl -lcrypto -pthread

client1: dropboxClient.o dropboxUtils.o
	gcc -o CLIENT dropboxClient.o dropboxUtils.o -lssl -lcrypto -pthread

dropboxServer.o: dropboxServer.c
	gcc -c dropboxServer.c -lssl -lcrypto -pthread

dropboxClient.o: dropboxClient.c
	gcc -pthread -c dropboxClient.c -lssl -lcrypto

dropboxUtils.o: dropboxUtils.c
	gcc -c dropboxUtils.c

