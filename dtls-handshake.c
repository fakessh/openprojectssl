#include <gnutls/gnutls.h>
#include <gnutls/dtls.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <wait.h>


enum role {
	SERVER,
	CLIENT
} role;

int debug;
int nonblock;

const char* role_to_name(enum role role)
{
	if (role == SERVER) {
		return "server";
	} else {
		return "client";
	}
}

void logfn(int level, const char* s)
{
	if (debug) {
		fprintf(stderr, "%s|<%i> %s", role_to_name(role), level, s);
	}
}

void auditfn(gnutls_session_t session, const char* s)
{
	if (debug) {
		fprintf(stderr, "%s| %s", role_to_name(role), s);
	}
}

void drop(const char* packet)
{
	if (debug) {
		fprintf(stderr, "%s| dropping %s\n", role_to_name(role), packet);
	}
}


typedef struct {
	int count;
} filter_packet_state_t;

filter_packet_state_t state_packet_ServerHello = { 0 };
filter_packet_state_t state_packet_ServerKeyExchange = { 0 };
filter_packet_state_t state_packet_ServerHelloDone = { 0 };
filter_packet_state_t state_packet_ClientKeyExchange = { 0 };
filter_packet_state_t state_packet_ClientChangeCipherSpec = { 0 };
filter_packet_state_t state_packet_ClientFinished = { 0 };
filter_packet_state_t state_packet_ServerChangeCipherSpec = { 0 };
filter_packet_state_t state_packet_ServerFinished = { 0 };

typedef struct {
	gnutls_datum_t packets[3];
	int* order;
	int count;
} filter_permute_state_t;

filter_permute_state_t state_permute_ServerHello = { { { 0, 0 }, { 0, 0 }, { 0, 0 } }, 0, 0 };
filter_permute_state_t state_permute_ServerFinished = { { { 0, 0 }, { 0, 0 }, { 0, 0 } }, 0, 0 };
filter_permute_state_t state_permute_ClientFinished = { { { 0, 0 }, { 0, 0 }, { 0, 0 } }, 0, 0 };

typedef void (*filter_fn)(gnutls_transport_ptr_t, const unsigned char*, size_t);

filter_fn filter_chain[32];
int filter_current_idx;

void filter_clear_state()
{
	memset(&state_packet_ServerHello, 0, sizeof(state_packet_ServerHello));
	memset(&state_packet_ServerKeyExchange, 0, sizeof(state_packet_ServerKeyExchange));
	memset(&state_packet_ServerHelloDone, 0, sizeof(state_packet_ServerHelloDone));
	memset(&state_packet_ClientKeyExchange, 0, sizeof(state_packet_ClientKeyExchange));
	memset(&state_packet_ClientChangeCipherSpec, 0, sizeof(state_packet_ClientChangeCipherSpec));
	memset(&state_packet_ServerChangeCipherSpec, 0, sizeof(state_packet_ServerChangeCipherSpec));
	memset(&state_packet_ServerFinished, 0, sizeof(state_packet_ServerFinished));

	for (int i = 0; i < 3; i++) {
		if (state_permute_ServerHello.packets[i].data) {
			free(state_permute_ServerHello.packets[i].data);
		}
		if (state_permute_ServerFinished.packets[i].data) {
			free(state_permute_ServerFinished.packets[i].data);
		}
		if (state_permute_ClientFinished.packets[i].data) {
			free(state_permute_ClientFinished.packets[i].data);
		}
	}

	memset(&state_permute_ServerHello, 0, sizeof(state_permute_ServerHello));
	memset(&state_permute_ServerFinished, 0, sizeof(state_permute_ServerFinished));
	memset(&state_permute_ClientFinished, 0, sizeof(state_permute_ClientFinished));
}

void filter_run_next(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	filter_fn fn = filter_chain[filter_current_idx];
	filter_current_idx++;
	if (fn) {
		fn(fd, buffer, len);
	} else {
		send((long int) fd, buffer, len, 0);
	}
}



int match_ServerHello(const unsigned char* buffer, size_t len)
{
	return role == SERVER && len >= 13 + 1 && buffer[0] == 22 && buffer[13] == 2;
}

void filter_packet_ServerHello(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ServerHello(buffer, len) && state_packet_ServerHello.count++ < 3) {
		drop("Server Hello");
	} else {
		filter_run_next(fd, buffer, len);
	}
}

int match_ServerKeyExchange(const unsigned char* buffer, size_t len)
{
	return role == SERVER && len >= 13 + 1 && buffer[0] == 22 && buffer[13] == 12;
}

void filter_packet_ServerKeyExchange(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ServerKeyExchange(buffer, len) && state_packet_ServerKeyExchange.count++ < 3) {
		drop("Server Key Exchange");
	} else {
		filter_run_next(fd, buffer, len);
	}
}

int match_ServerHelloDone(const unsigned char* buffer, size_t len)
{
	return role == SERVER && len >= 13 + 1 && buffer[0] == 22 && buffer[13] == 14;
}

void filter_packet_ServerHelloDone(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ServerHelloDone(buffer, len) && state_packet_ServerHelloDone.count++ < 3) {
		drop("Server Hello Done");
	} else {
		filter_run_next(fd, buffer, len);
	}
}

int match_ClientKeyExchange(const unsigned char* buffer, size_t len)
{
	return role == CLIENT && len >= 13 + 1 && buffer[0] == 22 && buffer[13] == 16;
}

void filter_packet_ClientKeyExchange(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ClientKeyExchange(buffer, len) && state_packet_ClientKeyExchange.count++ < 3) {
		drop("Client Key Exchange");
	} else {
		filter_run_next(fd, buffer, len);
	}
}

int match_ClientChangeCipherSpec(const unsigned char* buffer, size_t len)
{
	return role == CLIENT && len >= 13 && buffer[0] == 20;
}

void filter_packet_ClientChangeCipherSpec(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ClientChangeCipherSpec(buffer, len) && state_packet_ClientChangeCipherSpec.count++ < 3) {
		drop("Client Change Cipher Spec");
	} else {
		filter_run_next(fd, buffer, len);
	}
}

int match_ClientFinished(const unsigned char* buffer, size_t len)
{
	return role == CLIENT && len >= 13 && buffer[0] == 22 && buffer[4] == 1;
}

void filter_packet_ClientFinished(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ClientFinished(buffer, len) && state_packet_ClientFinished.count++ < 3) {
		drop("Client Finished");
	} else {
		filter_run_next(fd, buffer, len);
	}
}

int match_ServerChangeCipherSpec(const unsigned char* buffer, size_t len)
{
	return role == SERVER && len >= 13 && buffer[0] == 20;
}

void filter_packet_ServerChangeCipherSpec(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ServerChangeCipherSpec(buffer, len) && state_packet_ServerChangeCipherSpec.count++ < 3) {
		drop("Server Change Cipher Spec");
	} else {
		filter_run_next(fd, buffer, len);
	}
}

int match_ServerFinished(const unsigned char* buffer, size_t len)
{
	return role == SERVER && len >= 13 && buffer[0] == 22 && buffer[4] == 1;
}

void filter_packet_ServerFinished(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ServerFinished(buffer, len) && state_packet_ServerFinished.count++ < 3) {
		drop("Server Finished");
	} else {
		filter_run_next(fd, buffer, len);
	}
}



void filter_permutete_state_free_buffer(filter_permute_state_t* state)
{
	int i;

	for (i = 0; i < 3; i++) {
		if (state->packets[i].data) {
			free(state->packets[i].data);
		}
	}
}

void filter_permute_state_run(filter_permute_state_t* state, int packetCount,
		gnutls_transport_ptr_t fd, const unsigned char* buffer, size_t len)
{
	unsigned char* data = malloc(len);
	int packet = state->order[state->count];

	memcpy(data, buffer, len);
	state->packets[packet].data = data;
	state->packets[packet].size = len;
	state->count++;

	if (state->count == packetCount) {
		for (packet = 0; packet < packetCount; packet++) {
			filter_run_next(fd, state->packets[packet].data,
					state->packets[packet].size);
		}
		filter_permutete_state_free_buffer(state);
		state->count = 0;
	}
}

void filter_permute_ServerHello(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ServerHello(buffer, len)
			|| match_ServerKeyExchange(buffer, len)
			|| match_ServerHelloDone(buffer, len)) {
		filter_permute_state_run(&state_permute_ServerHello, 3, fd, buffer, len);
	} else {
		filter_run_next(fd, buffer, len);
	}
}

void filter_permute_ServerFinished(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ServerChangeCipherSpec(buffer, len)
			|| match_ServerFinished(buffer, len)) {
		filter_permute_state_run(&state_permute_ServerFinished, 2, fd, buffer, len);
	} else {
		filter_run_next(fd, buffer, len);
	}
}

void filter_permute_ClientFinished(gnutls_transport_ptr_t fd,
		const unsigned char* buffer, size_t len)
{
	if (match_ClientKeyExchange(buffer, len)
			|| match_ClientChangeCipherSpec(buffer, len)
			|| match_ClientFinished(buffer, len)) {
		filter_permute_state_run(&state_permute_ClientFinished, 3, fd, buffer, len);
	} else {
		filter_run_next(fd, buffer, len);
	}
}




ssize_t writefn(gnutls_transport_ptr_t fd, const void* buffer, size_t len)
{
	filter_current_idx = 0;
	filter_run_next(fd, (const unsigned char*) buffer, len);
	return len;
}

void await(int fd)
{
	if (nonblock) {
		struct pollfd p = { fd, POLLIN, 0 };
		poll(&p, 1, 100);
	}
}




gnutls_session_t session(int sock, int server)
{
	gnutls_session_t r;

	gnutls_init(&r, GNUTLS_DATAGRAM | (server ? GNUTLS_SERVER : GNUTLS_CLIENT)
			| GNUTLS_NONBLOCK * nonblock);
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

	gnutls_transport_set_push_function(r, writefn);

	gnutls_dtls_set_mtu(r, 1400);
	gnutls_dtls_set_timeouts(r, 100, 60000);

	return r;
}

int log(int code)
{
	if (code < 0 && code != GNUTLS_E_AGAIN) {
		fprintf(stderr, "<%s tls> %s", role_to_name(role), gnutls_strerror(code));
		if (gnutls_error_is_fatal(code)) {
			fprintf(stderr, " (fatal)\n");
			exit(1);
		} else {
			fprintf(stderr, "\n");
		}
	}
	return code;
}

timer_t killtimer_tid;

void reset_killtimer()
{
	if (nonblock) {
		return;
	}
	struct itimerspec tout = { { 0, 0 }, { 120, 0 } };
	timer_settime(killtimer_tid, 0, &tout, 0);
}

void setup_killtimer()
{
	struct sigevent sig;

	memset(&sig, 0, sizeof(sig));
	sig.sigev_notify = SIGEV_SIGNAL;
	sig.sigev_signo = 15;
	timer_create(CLOCK_MONOTONIC, &sig, &killtimer_tid);

	struct itimerspec tout = { { 0, 0 }, { 240, 0 } };
	timer_settime(killtimer_tid, 0, &tout, 0);
}

void log_error(int err, time_t started)
{
	if (err < 0) {
		if (err != GNUTLS_E_TIMEDOUT || (time(0) - started) >= 60) {
			log(err);
		} else {
			fprintf(stderr, "{spurious}");
			log(err);
		}
		if (gnutls_error_is_fatal(err)) {
			exit(1);
		}
	}
}

void client(int sock)
{
	gnutls_session_t s = session(sock, 0);
	int err = 0;
	time_t started = time(0);

	setup_killtimer();

	do {
		await(sock);
		err = log(gnutls_handshake(s));
		reset_killtimer();
	} while (err != 0 && !gnutls_error_is_fatal(err));
	log_error(err, started);

	started = time(0);
	const char* line = "foobar!";
	do {
		err = gnutls_record_send(s, line, strlen(line));
		reset_killtimer();
	} while (err < 0 && !gnutls_error_is_fatal(err));
	log_error(err, started);

	char buffer[8192];
	int len;
	do {
		await(sock);
		len = gnutls_record_recv(s, buffer, sizeof(buffer));
	} while (len < 0 && !gnutls_error_is_fatal(len));
	if (len > 0 && strcmp(line, buffer) == 0) {
		exit(0);
	} else {
		log(len);
		exit(1);
	}
}

void server(int sock)
{ 
	gnutls_session_t s = session(sock, 1);
	int err;
	time_t started = time(0);

	write(sock, &sock, 1);

	setup_killtimer();

	do {
		await(sock);
		err = log(gnutls_handshake(s));
		reset_killtimer();
	} while (err != 0 && !gnutls_error_is_fatal(err));
	log_error(err, started);

	for (;;) {
		char buffer[8192];
		int len;
		do {
			await(sock);
			len = gnutls_record_recv(s, buffer, sizeof(buffer));
			reset_killtimer();
		} while (len < 0 && !gnutls_error_is_fatal(len));
		log_error(len, started);

		gnutls_record_send(s, buffer, len);
		exit(0);
	}
}

void udp_sockpair(int* socks)
{
	struct sockaddr_in6 sa = { AF_INET6, htons(30000), 0, in6addr_loopback, 0 };
	struct sockaddr_in6 sb = { AF_INET6, htons(20000), 0, in6addr_loopback, 0 };

	socks[0] = socket(AF_INET6, SOCK_DGRAM, 0);
	socks[1] = socket(AF_INET6, SOCK_DGRAM, 0);

	bind(socks[0], (struct sockaddr*) &sa, sizeof(sa));
	bind(socks[1], (struct sockaddr*) &sb, sizeof(sb));

	connect(socks[1], (struct sockaddr*) &sa, sizeof(sa));
	connect(socks[0], (struct sockaddr*) &sb, sizeof(sb));
}

int run_test()
{
	int fds[2];
	int pid1, pid2;
	int status2;

	socketpair(AF_LOCAL, SOCK_DGRAM, 0, fds);

	if (nonblock) {
		fcntl(fds[0], F_SETFL, (long) O_NONBLOCK);
		fcntl(fds[1], F_SETFL, (long) O_NONBLOCK);
	}

	if (!(pid1 = fork())) {
		setpgrp();
		role = SERVER;
		server(fds[1]);
	}
	read(fds[0], &status2, sizeof(status2));
	if (!(pid2 = fork())) {
		setpgrp();
		role = CLIENT;
		client(fds[0]);
	}
	waitpid(pid2, &status2, 0);
	kill(pid1, 15);
	waitpid(pid1, 0, 0);

	close(fds[0]);
	close(fds[1]);

	if (WIFEXITED(status2)) {
		return !!WEXITSTATUS(status2);
	} else {
		return 2;
	}
}

void run_tests(int childcount)
{
	static int permutations2[2][2]
		= { { 0, 1 }, { 1, 0 } };
	static const char* permutations2names[2]
		= { "01", "10" };
	static int permutations3[6][3]
		= { { 0, 1, 2 }, { 0, 2, 1 },
			{ 1, 0, 2 }, { 1, 2, 0 },
			{ 2, 0, 1 }, { 2, 1, 0 } };
	static const char* permutations3names[6]
		= { "012", "021", "102", "120", "201", "210" };
	static filter_fn filters[8]
		= { filter_packet_ServerHello,
			filter_packet_ServerKeyExchange,
			filter_packet_ServerHelloDone,
			filter_packet_ClientKeyExchange,
			filter_packet_ClientChangeCipherSpec,
			filter_packet_ClientFinished,
			filter_packet_ServerChangeCipherSpec,
			filter_packet_ServerFinished };
	static const char* filter_names[8]
		= { "SHello",
			"SKeyExchange",
			"SHelloDone",
			"CKeyExchange",
			"CChangeCipherSpec",
			"CFinished",
			"SChangeCipherSpec",
			"SFinished" };

	int children = 0;

	for (int dropMode = 0; dropMode != 1 << 8; dropMode++)
	for (int serverFinishedPermute = 0; serverFinishedPermute < 2; serverFinishedPermute++)
	for (int serverHelloPermute = 0; serverHelloPermute < 6; serverHelloPermute++)
	for (int clientFinishedPermute = 0; clientFinishedPermute < 6; clientFinishedPermute++) {
		int fnIdx = 0;

		filter_clear_state();

		filter_chain[fnIdx++] = filter_permute_ServerHello;
		state_permute_ServerHello.order = permutations3[serverHelloPermute];

		filter_chain[fnIdx++] = filter_permute_ServerFinished;
		state_permute_ServerFinished.order = permutations2[serverFinishedPermute];

		filter_chain[fnIdx++] = filter_permute_ClientFinished;
		state_permute_ClientFinished.order = permutations3[clientFinishedPermute];

		if (dropMode) {
			for (int filterIdx = 0; filterIdx < 8; filterIdx++) {
				if (dropMode & (1 << filterIdx)) {
					filter_chain[fnIdx++] = filters[filterIdx];
				}
			}
		}
		filter_chain[fnIdx++] = NULL;

		if (!fork()) {
			int res = run_test();

			switch (res) {
				case 0:
					fprintf(stdout, "++ ");
					break;
				case 1:
					fprintf(stdout, "-- ");
					break;
				case 2:
					fprintf(stdout, "!! ");
					break;
			}

			fprintf(stdout, "SHello(%s), ", permutations3names[serverHelloPermute]);
			fprintf(stdout, "SFinished(%s), ", permutations2names[serverFinishedPermute]);
			fprintf(stdout, "CFinished(%s) :- ", permutations3names[clientFinishedPermute]);
			if (dropMode) {
				for (int filterIdx = 0; filterIdx < 8; filterIdx++) {
					if (dropMode & (1 << filterIdx)) {
						if (dropMode & ((1 << filterIdx) - 1)) {
							fprintf(stdout, ", ");
						}
						fprintf(stdout, "%s", filter_names[filterIdx]);
					}
				}
			}
			fprintf(stdout, "\n");

			if (res && debug) {
				exit(1);
			} else {
				exit(0);
			}
		} else {
			children++;
			while (children >= childcount) {
				wait(0);
				children--;
			}
		}
	}

	while (children > 0) {
		wait(0);
		children--;
	}
}



int main()
{
	gnutls_global_init();
	gnutls_global_set_log_function(logfn);
	gnutls_global_set_audit_log_function(auditfn);
	gnutls_global_set_log_level(-99);

	nonblock = 0;
	debug = 0;
	run_tests(1000);

	gnutls_global_deinit();
}

