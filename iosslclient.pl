#!/usr/bin/perl -w
# librement inspiré de la documentation perl
# auteur : fakessh @
use strict;
use IO::Socket::SSL 'inet4';

my ($host, $port, $kidpid, $handle, $line);

unless (@ARGV == 2) { die "usage: $0 host port" }
($host, $port) = @ARGV;
$IO::Socket::SSL::DEBUG = 3;
# Check to make sure that we were not accidentally run in the wrong
# directory:
unless (-d "certs") {
        if (-d "../certs") {
	    chdir "..";
	 } else {
     die "Please run this example from the IO::Socket::SSL distribution directory!\n";
	 }
 }
# cree une connexion tcp a l'hote et sur le port specifie
$handle = IO::Socket::SSL->new(Proto     => "tcp",
                                    SSL_use_cert => 1,
                                    SSL_verify_mode => 0x01,
                                    SSL_passwd_cb => sub { return "opossum" },
                                    PeerAddr  => $host,
                                    PeerPort  => $port)
           or die "can't connect to port $port on $host: $!";
     
print STDERR "[Connected to $host:$port]\n";
my ($subject_name, $issuer_name, $cipher);
if( ref($handle) eq "IO::Socket::SSL") {
        $subject_name = $handle->peer_certificate("subject");
        $issuer_name = $handle->peer_certificate("issuer");
        $cipher = $handle->get_cipher();
    }
warn "cipher: $cipher.\n", "server cert:\n", 
      "\t '$subject_name' \n\t '$issuer_name'.\n\n";

    # coupe le programme en deux processus, jumeaux identiques
die "can't fork: $!" unless defined($kidpid = fork());

    # le bloc if{} n'est traverse que dans le processus pere
if ($kidpid) {
        # copie la socket sur la sortie standard
        while (defined ($line = <$handle>)) {
            print STDOUT $line;
        }
        kill("TERM", $kidpid);          # envoie SIGTERM au fils
    }
    # le bloc else{} n'est traverse que dans le fils
    else {
        # copie l'entree standard sur la socket
        while (defined ($line = <STDIN>)) {
            print $handle $line;
        }
}

