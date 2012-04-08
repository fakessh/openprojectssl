#!/usr/bin/perl -w

use strict;
use IO::Socket::INET;
use IO::Socket::SSL;
use FindBin qw($Bin);
use IO::Select;
use Sys::Hostname qw(hostname);
use Data::Dumper;

use Net::SSLeay qw(die_now die_if_ssl_error);
Net::SSLeay::load_error_strings();
Net::SSLeay::SSLeay_add_ssl_algorithms();
Net::SSLeay::randomize();
$Net::SSLeay::trace = 2;

my $address = "localhost";
my $port = 6700;

$| = 1;

if ( my $pid = fork() ) {
    sleep 1;
    # parent.  Connect to the child.
    my $socket = IO::Socket::INET->new(
        PeerAddr      => $address,
        PeerPort      => $port,
        Timeout => 3,
        ) or die "socket connect failed; $!";

    print "client: connected\n";

    my $ctx;
    $ctx = Net::SSLeay::CTX_new()
        or die_now "CTX_new ($ctx) ($!)";
    Net::SSLeay::CTX_set_default_passwd_cb($ctx, sub {'secr1t'});
    #Net::SSLeay::set_options($ssl, &Net::SSLeay::OP_ALL)
            #and die_if_ssl_error("ssl set options");
    #Net::SSLeay::CTX_use_RSAPrivateKey_file
            #($ctx, 'certs/client-key.pem',
             #&Net::SSLeay::FILETYPE_PEM);
    #die_if_ssl_error("private key");
    #Net::SSLeay::CTX_use_certificate_file
            #($ctx, 'certs/client-cert.pem',
             #&Net::SSLeay::FILETYPE_PEM);
    #die_if_ssl_error("certificate");
    Net::SSLeay::set_cert_and_key(
        $ctx, "$Bin/client-cert.pem",
        "$Bin/client-key.pem") or die "key";
    Net::SSLeay::CTX_load_verify_locations($ctx, '', $Bin)
            or die_now("CTX load verify loc=`$Bin' $!");
    Net::SSLeay::CTX_set_verify($ctx, &Net::SSLeay::VERIFY_PEER,sub{1});

    my $ssl = Net::SSLeay::new($ctx);

    Net::SSLeay::set_fd($ssl, fileno($socket));
    my $res = Net::SSLeay::connect($ssl)
        and die_if_ssl_error("ssl connect");
    print "client: ssl open, cipher `"
        . Net::SSLeay::get_cipher($ssl) . "'\n";
    print "client: server cert: "
        . Net::SSLeay::dump_peer_certificate($ssl);

    $res =  Net::SSLeay::write($ssl, "Hello, SSL\n");
    die_if_ssl_error("ssl write");
    CORE::shutdown $socket, 1;
    my $got = Net::SSLeay::read($ssl);
    die_if_ssl_error("ssl read");
    print $got;
    Net::SSLeay::free ($ssl);
    Net::SSLeay::CTX_free ($ctx);
    close $socket;

    print "client: waiting for child process\n";
    wait;
}
elsif ( defined $pid ) {
    $Net::SSLeay::trace = 3;
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
    $ctx = Net::SSLeay::CTX_new()
        or die_now "CTX_new ($ctx) ($!)";
    Net::SSLeay::CTX_set_default_passwd_cb($ctx, sub {'secr1t'});
    Net::SSLeay::set_cert_and_key(
        $ctx, "$Bin/server-cert.pem",
        "$Bin/server-key.pem") or die "key";
    Net::SSLeay::CTX_load_verify_locations($ctx, '', $Bin)
            or die_now("CTX load verify loc=`$Bin' $!");
    Net::SSLeay::CTX_set_verify($ctx, &Net::SSLeay::VERIFY_PEER,sub{
                my ($ok, $ctx_store) = @_;
                my ($certname,$cert,$error);
                if ($ctx_store) {
                        $cert =
Net::SSLeay::X509_STORE_CTX_get_current_cert($ctx_store);
                        $error =
Net::SSLeay::X509_STORE_CTX_get_error($ctx_store);
                        $certname =
Net::SSLeay::X509_NAME_oneline(Net::SSLeay::X509_get_issuer_name($cert)).Net::SSLeay::X509_NAME_oneline(Net::SSLeay::X509_get_subject_name($cert));
                        $error &&= Net::SSLeay::ERR_error_string($error);
                }
                print
"verify_cb->($ok,$ctx_store,$certname,$error,$cert);\n";
        1;
    });

    my $ssl = Net::SSLeay::new($ctx);

    Net::SSLeay::set_fd($ssl, fileno($client));
    my $res = Net::SSLeay::accept($ssl)
        and die_if_ssl_error("ssl accept");
    print "server: ssl open, cipher `"
        . Net::SSLeay::get_cipher($ssl) . "'\n";
    print "server: client cert: "
        . Net::SSLeay::dump_peer_certificate($ssl);
    my $s = IO::Select->new;
    $s->add($client);
    while ( $s->can_read ) {
        my $data;
        my $got = Net::SSLeay::ssl_read_all($ssl, 1024);
        if ( length($got) == 0 ) {
            last;
        }
        else {
            print "server: read $got\n";
        }
    }
    Net::SSLeay::free ($ssl);
    Net::SSLeay::CTX_free ($ctx);
    $client->shutdown(1);
}
else {
  die "fork failed; $!";
}