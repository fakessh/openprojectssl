#!/usr/bin/perl -w

use Carp;
use Net::SMTP::Server;
use Net::SMTP::Server::Client;
use Net::SMTP::Server::Relay;

$server = new Net::SMTP::Server('localhost.localdomain',25) ||
  croak ("Unable to handle client connection : $!\n");

while ( $conn = $server->accept()) {
    
    # We can perform all sorts of checks here for spammers , ACL,
    # and other usefum stuff to check on a connection
    
    # Handle the client's connection and spawn off a new parser.
    # This can / should be a fork() or a new thread,
    # but for simplicity
    my $client = new Net::SMTP::Server::Client($conn) || 
      croak ("Unable to handle client connection : $!\n");
    
    # Process the client. This command will block until 
    # the connecting client completes the SMTP transcation
    $client->process || next;
    
    my $relay = new Net::SMTP::Server::Relay($client->{FROM},
	                                     $client->{TO},
	                                     $client->{MSG});
    
    }
    
    
