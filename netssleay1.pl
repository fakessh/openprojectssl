#!usr/bin/perl -w

use strict;
use IO::Socket::INET;
use FindBin qw($Bin);
use IO::Select;
use Sys::Hostname qw(hostname);
use Data::Dumper;

use Net::SSLeay::Context;
use Net::SSLeay::SSL;
use Net::SSLeay::Constants qw(OP_ALL OP_NO_SSLv2 FILETYPE_PEM VERIFY_PEER);

my $address = "localhost";
my $port = 42015;

$| = 1;

if ( my $pid = fork() ) {
    sleep 1;
    #parent Connect to the child
    my $socket = IO::Socket::INET->new (
	PeerAddr    =>   $address,
	PeerPort    =>   $port,
	Timeout     =>   3,
	) or die "socket connect failed: $!";
    
    print "client: connected\n";
    
    my $ctx = Net::SSLeay::Context->new;
    $ctx->set_default_passwd_cb( sub { 'secr1t'} );
    $ctx->set_options(OP_ALL && OP_NO_SSLv2);
    $ctx->use_certificate_chain_file('certs/client-cert.pem');
    $ctx->load_verify_locations( '' ,$Bin);
    $ctx->set_verify(VERIFY_PEER);
    
    my $ssl = Net::SSLeay::SSL->new ( ctx => $ctx );
    $ssl->set_fd(fileno($socket));
    my $res = $ssl->connect();
    print "client: ssl open, cipher " .  $ssl->get_cipher . "\n";
    print "client: server cert: " . $ssl->dump_peer_certificate;

    $res =  $ssl->write($ssl, "Hello, SSL\n");
    CORE::shutdown $socket, 1;
    my $got = $ssl->read();
    die_if_ssl_error("ssl read");
    print $got;
    undef($ssl);
    close $socket;
    
    print "client: waiting for child process\n";
    wait;
}
elsif ( defined $pid ) {
    # child.  start the server.
   my $sock = IO::Socket::INET->new(
	 LocalAddr => $address,
	 LocalPort => $port,
         Listen => 2,
         Reuse => 1,
         Proto => "tcp",
             ) or die $!;
    print "server: listening on port $port\n";
    my $client = $sock->accept();
    print "server: accepted a socket\n";
    my $ctx;
    $ctx = Net::SSLeay::Context->new();
    $ctx->set_default_passwd_cb(sub{'secr1t'});
    $ctx->use_certificate_chain_file('certs/server-cert.pem');
    $ctx->use_RSAPrivateKey_file('certs/server-key.pem', FILETYPE_PEM);
    $ctx->load_verify_locations('', $Bin);
    $ctx->set_verify(
        VERIFY_PEER, sub {
	  my ($ok, $ctx_store) = @_;
	  my ($certname,$cert,$error);
	  if ($ctx_store) {
	     $cert = $ctx_store->get_current_cert;
	     $error = $ctx_store->get_error;
	     $certname = $cert->get_issuer_name->oneline
	     . $cert->get_subject_name->oneline;
	     $error &&= $error->error_string;
             }
	     print "verify_cb->($ok,$ctx_store,$certname,$error,$cert);\n";
             1;
         });
    my $ssl = Net::SSLeay::SSL->new(ctx => $ctx);
    $ssl->set_fd(fileno($client));
    my $res = $ssl->accept();
    print "server: ssl open, cipher " . $ssl->get_cipher . "'\n";
    print "server: client cert: " . $ssl->dump_peer_certificate;
    my $s = IO::Select->new;
    $s->add($client);
    while ( $s->can_read ) {
	my $data;
	my $got = $ssl->ssl_read_all(1024);
	if ( length($got) == 0 ) {
	                last;
	    }
       else {
                    print "server: read $got\n";
                }
       }
    undef($ssl);
    $client->shutdown(1);
    }
else {
    die "fork failed; $!";
}
