
#!/usr/bin/perl

use strict;
use Net::SMTP::Server;
use Net::SMTP::Server::Client;
use Data::Dumper;
use Carp qw( croak );

our $host = $ARGV[0] || "0.0.0.0" ;

our $server = new Net::SMTP::Server($host) || 
  croak("Unable to open SMTP server socket");

print "Waiting for incoming SMTP connections on ".($host eq "0.0.0.0" ? "all IP addresses":$host)."\n";
$| = 1;
while(my $conn = $server->accept()) {
  print "Incoming mail ... from " . $conn->peerhost() ;
  my $client = new Net::SMTP::Server::Client($conn) || 
    croak("Unable to process incoming request");
  if (!$client->process) { 
    print "\nfailed to receive complete e-mail message\n"; next; }
  print " received\n";
  print "From: $client->{FROM}\n";
  my $to = $client->{TO};
  my $toList = @{$to} ? join(",",@{$to}) : $to;
  print "To:   $toList\n";
  print "\n" ;
  print $client->{MSG};
}
