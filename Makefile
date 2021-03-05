
CC=g++
CFLAGS=-I.


all: server client

server: server.o Csocket.o CClient.o CRoom.o
	$(CC) -o $@ $^ $(CFLAGS)

client: client.o Csocket.o
	$(CC) -o $@ $^ $(CFLAGS)

server.o: server.cpp Csocket.h CClient.h CRoom.h
	$(CC) -c -o $@ $< $(CFLAGS)

client.o: client.cpp Csocket.h
	$(CC) -c -o $@ $< $(CFLAGS)

Csocket.o: Csocket.cpp Csocket.h
	$(CC) -c -o $@ $< $(CFLAGS)

CClient.o: CClient.cpp CClient.h
	$(CC) -c -o $@ $< $(CFLAGS)

CRoom.o: CRoom.cpp CRoom.h
	$(CC) -c -o $@ $< $(CFLAGS)


clean:
	rm *.o 
