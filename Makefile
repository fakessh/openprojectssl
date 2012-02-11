# project client server openssl c 
# with gabriel on chili
# write fakessh@fakessh.eu
# date vendredi 26 novembre 13:27:10 CET 2010


PATHSYSINCL := /usr/include
PATHSSLLIB := /usr/lib
SSLLIBS := -lssl -lcrypto -pthread 
GNUTLSLIBS := -lgnutls
CC := gcc
CFLAGS := -g -Wextra
CLIENTBINHTTP := sslclienthttp
CLIENTBIN := sslclient
SERVERBIN := sslserver
ECHOSERVERGNUTLS := echoservergnutls
ECHOSERVERGNUTLSOPENPGP := echoservergnutlsopenpgp

all: sslserver sslclient echoservergnutls echoservergnutlsopenpgp

sslserver: sslServer.o  common.o
	$(CC) $^ -o $(SERVERBIN) -L$(PATHSSLLIB) $(SSLLIBS)
sslclient: sslClient.o  commonclient.o
	$(CC) $^ -o $(CLIENTBIN) -L$(PATHSSLLIB) $(SSLLIBS)
echoservergnutls: echoservergnutls.o
        $(CC) $^ -o $(ECHOSERVERGNUTLS) -L$(PATHSSLLIB) $(GNUTLSLIBS)
echoservergnutlsopenpgp: echoservergnutlsopenpgp.o
        $(CC) $^ -o $(ECHOSERVERGNUTLSOPENPGP) -L$(PATHSSLLIB) $(GNUTLSLIBS)
	
sslClient.o : sslClient.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
sslServer.o : sslServer.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
echo.o : echo.c
	$(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
echoservergnutls.o : echoservergnutls.c
        $(CC) $(CFLAGS) -I$(PATHSYSINCL) -Wall -c -o $@ $<
echoservergnutlsopenpgp.o : echoservergnutlsopenpgp.c
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
	mv $(ECHOSERVERGNUTLS) bin
	mv $(ECHOSERVERGNUTLSOPENPGP) bin
clean:
	clear
	rm -f *.o
	rm -f $(CLIENTBIN)
	rm -f $(SERVERBIN)
	rm -f $(ECHOSERVERGNUTLS)
	rm -f $(ECHOSERVERGNUTLSOPENPGP)
	rm -f bin/*
	rmdir bin
