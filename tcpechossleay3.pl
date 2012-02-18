#!/usr/bin/perl -w
# tcpecho.pl - Echo server using TCP
#
# Copyright (c) 2003 Sampo Kellomaki <sampo@iki.fi>, All Rights Reserved.
# $Id: tcpecho.pl,v 1.2 2003/08/17 07:44:47 sampo Exp $
# 17.8.2003, created --Sampo
#
# Usage: ./tcpecho.pl *port*
#
# This server always binds to localhost as this is all that is needed
# for tests.

die "Usage: ./sslecho.pl *port* *cert.pem* *key.pem*\n" unless $#ARGV == 2;
my($port, $cert_pem, $key_pem) = @ARGV;
my $our_ip = "\x7F\0\0\x01";

my $trace = 2;
my $debug = 2;
my $ctx;
my $fd;
my $addr;
my $old_out;

use Socket;
use warnings;
use strict;
use Errno;
use Fcntl;

use Net::SSLeay qw(die_now die_if_ssl_error);
Net::SSLeay::load_error_strings();
Net::SSLeay::SSLeay_add_ssl_algorithms();


# #############
# fonction ####
# #############

# #################
# start_listening # 
# #################

# #########################################
# Create the socket and open a connection #
# #########################################

sub start_listening {
        my ($port) = @_;
        my ($socket,$our_ip,$our_serv_params);
##        $our_serv_params = pack ('S n a4 x8', &AF_INET, $port, $our_ip);

        socket($socket, PF_INET, SOCK_STREAM, getprotobyname("tcp")) or die("socket: $!\n");
        setsockopt($socket, SOL_SOCKET, SO_REUSEADDR, 1) or die("setsockopt SOL_SOCKET, SO_REUSEADDR: $!\n");
        bind($socket, pack_sockaddr_in($port, INADDR_ANY)) or die("bind ${port}: $!\n");
        listen($socket, SOMAXCONN) or die("listen ${port}: $!\n");

        # Set FD_CLOEXEC.
        $_ = fcntl($socket, F_GETFD, 0) or die "fcntl: $!\n";
        fcntl($socket, F_SETFD, $_ | FD_CLOEXEC) or die "fnctl: $!\n";

        return $socket;
}


# #############
# fonction ####
# #############

# #################
# myaccept ######## 
# #################

# ##########################################################
# Create the myaccept non blocking function for connection #
# ##########################################################

sub myaccept {
            my ($socket) = @_;
            die "accept: $!\n" unless accept(my $connection, $socket);
            $_ = select($connection); $| = 1; select $_;
    
            # Set FD_CLOEXEC.
            $_ = fcntl($connection, F_GETFD, 0) or die "fcntl: $!\n";
            fcntl($connection, F_SETFD, $_ | FD_CLOEXEC) or die "fnctl: $!\n";
    
            # Set O_NONBLOCK.
            $_ = fcntl($connection, F_GETFL, 0) or die "fcntl F_GETFL: $!\n";  # 0 for error, 0e0 for 0.
            fcntl($connection, F_SETFL, $_ | O_NONBLOCK) or die "fcntl F_SETFL O_NONBLOCK: $!\n";  # 0 for error, 0e0 for 0.
            return $connection;
}


# Net::SSLeay's error functions are terrible. These are a bit more programmable and readable.
sub ssl_get_error {
        my $errors = "";
        my $errnos = [];
        while(my $errno = Net::SSLeay::ERR_get_error()) {
                push @$errnos, $errno;
                $errors .= Net::SSLeay::ERR_error_string($errno) . "\n";
        }
        return $errors, $errnos if wantarray;
        return $errors;
}

sub ssl_check_die {
        my ($message) = @_;
        my ($errors, $errnos) = ssl_get_error();
        die "${message}: ${errors}" if @$errnos;
        return;
}

#
# Create the socket and open a connection
#
Net::SSLeay::load_error_strings();
Net::SSLeay::ERR_load_crypto_strings();
Net::SSLeay::SSLeay_add_ssl_algorithms();
Net::SSLeay::randomize();

print "sslecho: Creating SSL context...\n" if $trace>1;
$ctx = Net::SSLeay::CTX_new () or die_now("CTX_new ($ctx): $!\n");
print "sslecho: Setting cert and RSA key...\n" if $trace>1;
Net::SSLeay::CTX_set_cipher_list($ctx,'ALL');
Net::SSLeay::set_cert_and_key($ctx, $cert_pem, $key_pem) or die "key";


sub main {
        $| = 1;

        # The CTX ("context") should be shared between many SSL connections. A CTX
        # could apply to multiple listening sockets, or each listening socket could
        # have its own CTX. Each CTX may represent only one local certificate.
        my $ctx = Net::SSLeay::CTX_new();
        ssl_check_die("SSL CTX_new");

        # OP_ALL enables all harmless work-arounds for buggy clients.
        Net::SSLeay::CTX_set_options($ctx, Net::SSLeay::OP_ALL());
        ssl_check_die("SSL CTX_set_options");

        # Modes:
        # 0x1: SSL_MODE_ENABLE_PARTIAL_WRITE
        # 0x2: SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER
        # 0x4: SSL_MODE_AUTO_RETRY
        # 0x8: SSL_MODE_NO_AUTO_CHAIN
        # 0x10: SSL_MODE_RELEASE_BUFFERS (ignored before OpenSSL v1.0.0)
        Net::SSLeay::CTX_set_mode($ctx, 0x11);
        ssl_check_die("SSL CTX_set_mode");

        # Load certificate. This will prompt for a password if necessary.
        Net::SSLeay::CTX_use_RSAPrivateKey_file($ctx, $key_pem, Net::SSLeay::FILETYPE_PEM());
        ssl_check_die("SSL CTX_use_RSAPrivateKey_file");
        Net::SSLeay::CTX_use_certificate_file($ctx, $cert_pem, Net::SSLeay::FILETYPE_PEM());
        ssl_check_die("SSL CTX_use_certificate_file");

        my $socket = start_listening($port);
        while(1) {
                my $connection = myaccept($socket);

                # Each connection needs an SSL object, which is associated with the shared CTX.
                my $ssl = Net::SSLeay::new($ctx);
                ssl_check_die("SSL new");
                Net::SSLeay::set_fd($ssl, fileno($connection));
                ssl_check_die("SSL set_fd");
                Net::SSLeay::accept($ssl);
                ssl_check_die("SSL accept");
                my $http_header_get = "GET";
                my $http_header = "";
                print "Read returned" if $debug;
                until($http_header =~ qr~\n\r$http_header_get\n~s) {
                        my $vec = "";
                        vec($vec, fileno($connection), 1) = 1;
                        select($vec, undef, undef, undef);

                        # Repeat read() until EAGAIN before select()ing for more. SSL may already be
                        # holding the last packet in its buffer, so if we aren't careful to decode
                        # everything that's pending we could block forever at select(). This would be
                        # after SSL already read "\r\n\r\n" but before it decoded and returned it. As
                        # documented, OpenSSL returns data from only one SSL record per call, but its
                        # internal system call to read may gather more than one record. In short, a
                        # socket may not become readable again after reading, but note that writing
                        # doesn't have this problem since a socket will always become writable again
                        # after writing.

                        while(1) {
                                # 16384 is the maximum amount read() can return; larger values allocate memory
                                # that can't be unused as part of the buffer passed to read().
                                my $read_buffer = Net::SSLeay::read($ssl, 16384);
                                ssl_check_die("SSL read");
                                die "read: $!\n" unless defined $read_buffer or $!{EAGAIN} or $!{EINTR} or $!{ENOBUFS};
                                if(defined $read_buffer) {
                                        printf " %s bytes,", length($read_buffer) if $debug;
                                        $http_header .= $read_buffer;
                                } else {
                                        print " undef," if $debug;
                                        last;
                                }
                        }
                }
                print "\n" if $debug;

                print $http_header if $debug;

                my $write_buffer = "HTTP/1.0 200 OK\r\n\r\n" . `cat /usr/share/games/fortune/fortunes`;

                print "Write returned" if $debug;
                while(length($write_buffer)) {
                        my $vec = "";
                        vec($vec, fileno($connection), 1) = 1;
                        select(undef, $vec, undef, undef);

                        my $write = Net::SSLeay::write($ssl, $write_buffer);
                        ssl_check_die("SSL write");
                        die "write: $!\n" unless $write != -1 or $!{EAGAIN} or $!{EINTR} or $!{ENOBUFS};
                        print " ${write}," if $debug;
                        substr($write_buffer, 0, $write, "") if $write > 0;
                }
                print "\n" if $debug;

                # Paired with closing connection.
                Net::SSLeay::free($ssl);
                ssl_check_die("SSL free");
                close($connection);
        }

        # Paired with closing listening socket.
        Net::SSLeay::CTX_free($ctx);
        ssl_check_die("SSL CTX_free");
        close($socket);
}

exit main();

