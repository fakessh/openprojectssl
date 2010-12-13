# project client server openssl c 
# with gabriel on chili
# write fakessh@fakessh.eu
# date vendredi 26 novembre 13:27:10 CET 2010


PATHSYSINCL := /usr/include
PATHSSLLIB := /usr/lib
SSLLIBS := -lssl -lcrypto -pthread 
CC := gcc
CFLAGS := -g -Wextra
CLIENTBINHTTP := sslclienthttp
CLIENTBIN := sslclient
SERVERBIN := sslserver

all: sslserver sslclient

sslserver: sslServer.o  common.o
	$(CC) $^ -o $(SERVERBIN) -L$(PATHSSLLIB) $(SSLLIBS)

sslclient: sslClient.o  commonclient.o
	$(CC) $^ -o $(CLIENTBIN) -L$(PATHSSLLIB) $(SSLLIBS)

sslClient.o : sslClient.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
sslServer.o : sslServer.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
echo.o : echo.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
client.o : client.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
server.o : server.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
read_write.o : read_write.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
common.o : common.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
commonclient.o : commonclient.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<

install :
	mkdir bin
	mv $(CLIENTBIN) bin
	mv $(SERVERBIN) bin
clean:
	clear
	rm -f *.o
	rm -f $(CLIENTBIN)
	rm -f $(SERVERBIN)
	rm -f bin/*
	rmdir bin
