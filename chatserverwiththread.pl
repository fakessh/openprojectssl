#!/usr/bin/perl
use warnings;
use strict;
use IO::Socket::SSL 'inet4';
use threads;
use threads::shared;

my $user;

$|++;
print "$$ Server started\n";; # do a "top -p -H $$" to monitor server threads

our @clients : shared;
@clients = ();

my $server = new IO::Socket::SSL(
				 Timeout => 60,
				 Proto => "tcp",
				 LocalPort => 42000,
				 Reuse => 1,
				 Listen => 25,
				 SSL_verify_mode => 0x04,
				 SSL_key_file => '/etc/pki/tls/private/ks37777.kimsufi.com.key',
				 SSL_cert_file => '/etc/pki/tls/certs/ks37777.kimsufi.com.cert'
);

while (1) {
    my $client;

    do {
	$client = $server->accept;
    } until ( defined($client) );

    $server->recv($user, 300);

    my $peerhost = $client->peerhost();
    print "accepted a client $client, $peerhost, username = $user \n";
    my $fileno = fileno $client;
    push ( @clients, $fileno);
    #spawn a thread here for each client
    my $thr = threads->new( \&process_it, $client, $fileno,
			    $peerhost )->detach();
}
# end of main thread

sub process_it {
    my ($lclient,$lfileno,$lpeer) = @_; #local client

    if($lclient->connected){
    # Here you can do your stuff
    # I use have the server talk to the client
    # via print $client and while(<$lclient>)
	print $lclient "$lpeer->Welcome to server\n";

	while(<$lclient>){
        # print $lclient "$lpeer->$_\n";
	    print "clients-> @clients\n";

	    foreach my $fn ( @clients) {
		open my $fh, ">&=$fn" or warn $! and die;
		print $fh "$_"
		}
	}
}
#close filehandle before detached thread dies out
close( $lclient);
#remove multi-echo-clients from echo list
} 
