#include <iostream>
#include <sstream>
#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <wait.h>

using namespace std;

const char* prefix;

void logfn(int level, const char* s)
{
	std::cout << "<" << prefix << " tls-int " << level << "> " << s;
}

void auditfn(gnutls_session_t session, const char* s)
{
	std::cout << "<" << prefix << " tls-aud " << session << "> " << s;
}

ssize_t readfn(gnutls_transport_ptr_t sock, void* buf, size_t len)
{
	return read((int) (long long) sock, buf, len);
}



int ttype, packetCount = 0;
#define NONBLOCK 0

// server first flight droppers
// for three flights, drop any, but not all, server hello packets
bool dropServerHello1(const char* buf, size_t len)
{
	return packetCount < 10
		&& ttype == 0
		&& len >= 13 + 1
		&& buf[0] == 22 // handshake packet
		&& buf[13] == 2; // server hello
}

bool dropServerHello2(const char* buf, size_t len)
{
	return packetCount < 10
		&& ttype == 0
		&& len >= 13 + 1
		&& buf[0] == 22 // handshake packet
		&& buf[13] == 12; // server hello key exchange
}

bool dropServerHello3(const char* buf, size_t len)
{
	return packetCount < 10
		&& ttype == 0
		&& len >= 13 + 1
		&& buf[0] == 22 // handshake packet
		&& buf[13] == 14; // server hello done
}

bool dropServerHello12(const char* buf, size_t len)
{
	return dropServerHello1(buf, len) || dropServerHello2(buf, len);
}

bool dropServerHello13(const char* buf, size_t len)
{
	return dropServerHello1(buf, len) || dropServerHello3(buf, len);
}

bool dropServerHello23(const char* buf, size_t len)
{
	return dropServerHello2(buf, len) || dropServerHello3(buf, len);
}


// complete server hello dropper
// drop three flights of server hello packets
bool dropServerHello(const char* buf, size_t len)
{
	return dropServerHello12(buf, len) || dropServerHello3(buf, len);
}


// drop two ChangeCipherSpec packets
bool dropServerChangeCipherSpec(const char* buf, size_t len)
{ 
	return packetCount < 7
		&& ttype == 0
		&& len >= 13 
		&& buf[0] == 20; // change cipher spec packet
}

// drop two encrypted Finished packets
bool dropServerFinished(const char* buf, size_t len)
{
	return packetCount < 8
		&& ttype == 0
		&& len >= 13
		&& buf[0] == 22 // handshake packet
		&& buf[4] == 1; // server finished, encrypted (heuristic)
		                // server never sends anything else in epoch 1
}



// client drop functions
// drop three client key exchange packets
bool dropClientKeyExchange(const char* buf, size_t len)
{
	return packetCount < 8
		&& ttype == 1
		&& len >= 13 
		&& buf[0] == 22 // handshake packet
		&& buf[13] == 16; // key exchange packet
}

// drop two ChangeCipherSpec packets
bool dropClientChangeCipherSpec(const char* buf, size_t len)
{ 
	return packetCount < 8
		&& ttype == 1
		&& len >= 13 
		&& buf[0] == 20; // change cipher spec packet
}

// drop two encrypted Finished packets
bool dropClientFinished(const char* buf, size_t len)
{
	return packetCount < 8
		&& ttype == 1
		&& len >= 13
		&& buf[0] == 22 // handshake packet
		&& buf[4] == 1; // client finished, encrypted (heuristic)
		                // match packetCount precisely!
}

bool (*dropfn)(const char* buf, size_t len)
	= dropServerHello12;

ssize_t writefn(gnutls_transport_ptr_t sock, const void* buf, size_t len)
{
	packetCount++;
	if (dropfn && dropfn((const char*) buf, len)) {
		std::cout << prefix << ": dropping packet" << std::endl;
		errno = 0;
		return len;
	}
	std::cout << prefix << ": sending packet" << std::endl;

	return write((int) (long long) sock, buf, len);
}

int timeoutfn(gnutls_transport_ptr_t sock, unsigned int ms)
{
	char buffer[99999];
	int ret = recv((int) (long long) sock, buffer, sizeof(buffer), MSG_PEEK);

	if (ret >= 0) {
		return ret;
	} else if (errno == EAGAIN) {
		return 0;
	} else {
		return -1;
	}
}

int await(int sock)
{
#if NONBLOCK == 0
	return 0;
#else
	pollfd p = { sock, POLLIN, 0 };
	usleep(100);
	return poll(&p, 1, 1000);
#endif
}

gnutls_session_t session(int sock, bool server)
{
	gnutls_session_t r;

	gnutls_init(&r, GNUTLS_DATAGRAM | (server ? GNUTLS_SERVER : GNUTLS_CLIENT)
			| GNUTLS_NONBLOCK * NONBLOCK);
	gnutls_priority_set_direct(r, "NORMAL:+ANON-ECDH", 0);
	gnutls_transport_set_ptr(r, (gnutls_transport_ptr_t) sock);

	if (server) {
		gnutls_anon_server_credentials_t cred;
		gnutls_anon_allocate_server_credentials(&cred);
		gnutls_credentials_set(r, GNUTLS_CRD_ANON, cred);
	} else {
		gnutls_anon_client_credentials_t cred;
		gnutls_anon_allocate_client_credentials(&cred);
		gnutls_credentials_set(r, GNUTLS_CRD_ANON, cred);
	}

	gnutls_transport_set_pull_function(r, readfn);
	gnutls_transport_set_push_function(r, writefn);
	gnutls_transport_set_pull_timeout_function(r, timeoutfn);

	gnutls_dtls_set_mtu(r, 8192);

	return r;
}

int log(int code)
{
	cout << "<" << prefix << " tls> " << gnutls_strerror(code);
	if (gnutls_error_is_fatal(code)) {
		cout << " (fatal)" << endl;
		exit(1);
	} else {
		cout << endl;
	}
	return code;
}

void client(int sock)
{
	gnutls_session_t s = session(sock, false);

	while (log(gnutls_handshake(s))) {
		await(sock);
	}

	string line = "foobar!";
	gnutls_record_send(s, line.c_str(), line.size());
}

void server(int sock)
{ 
	gnutls_session_t s = session(sock, true);

	do {
		await(sock);
	} while (log(gnutls_handshake(s)));

	for (;;) {
		await(sock);

		char buffer[8192];
		int len = gnutls_record_recv(s, buffer, sizeof(buffer));

		if (len > 0) cout << "RECEIVED: " << buffer << endl;
	}

	dropfn = NULL;
	gnutls_bye(s, GNUTLS_SHUT_WR);
}

void udp_sockpair(int* socks)
{
	socks[0] = socket(AF_INET, SOCK_DGRAM, 0);
	socks[1] = socket(AF_INET, SOCK_DGRAM, 0);

	sockaddr_in sa = { AF_INET, 0xffff, 0x0100007f };
	sockaddr_in sb = { AF_INET, 0x0004, 0x0100007f };

	bind(socks[0], (sockaddr*) &sa, sizeof(sa));
	bind(socks[1], (sockaddr*) &sb, sizeof(sb));

	connect(socks[1], (sockaddr*) &sa, sizeof(sa));
	connect(socks[0], (sockaddr*) &sb, sizeof(sb));

	if (NONBLOCK) {
		fcntl(socks[0], F_SETFL, (long) O_NONBLOCK);
		fcntl(socks[1], F_SETFL, (long) O_NONBLOCK);
	}
}

int main()
{
	int socks[2];

	udp_sockpair(socks);

	gnutls_global_init();
	gnutls_global_set_log_function(logfn);
	gnutls_global_set_audit_log_function(auditfn);
	gnutls_global_set_log_level(99);

	int pid1, pid2;
	if (!(pid1 = fork())) {
		prefix = "server";
		ttype = 0;
		server(socks[1]);
	}
	if (!(pid2 = fork())) {
		prefix = "client";
		ttype = 1;
		client(socks[0]);
	}
	waitpid(-1, 0, 0);
	sleep(1);
	kill(pid1, 15);
	kill(pid2, 15);
}

