#!/usr/bin/perl -w
# librement inspire de la documentation perl
# autheur : fakessh @
 use IO::Socket::SSL 'inet4';
 use Net::hostent;              # pour la version OO de gethostbyaddr
 use strict;
 use warnings;
my  $PORT = 8000;                 

my $server = IO::Socket::SSL->new( Proto     => 'tcp',
                                  LocalPort => $PORT,
                                  Listen    => 5 ,
                                  SSL_verify_mode => 0x00,
                                  SSL_key_file    => '/tmp/ssl/key.txt',
                                  SSL_cert_file   => '/tmp/ssl/cert.txt',
                                  Reuse     => 1);

 die "can't setup server" unless $server;
 print "[Server $0 accepting clients]\n";
my $client;
 while ($client = $server->accept()) {
##   $client->autoflush(1);
   print $client "Welcome to $0; type help for command list.\n";
   my$hostinfo = gethostbyaddr($client->peeraddr);
   printf "[Connect from %s]\n", $hostinfo->name || $client->peerhost;
   print $client "Command? ";
   while ( <$client>) {
     next unless /\S/;       # ligne vide
     if    (/quit|exit/i)    { last;                                     }
     elsif (/date|time/i)    { printf $client "%s\n", scalar localtime;  }
     elsif (/who/i )         { print  $client `who 2>&1`;                }
     elsif (/cookie/i )      { print  $client `/usr/games/fortune 2>&1`; }
     elsif (/motd/i )        { print  $client `cat /etc/motd 2>&1`;      }
     else {
       print $client "Commands: quit date who cookie motd\n";
     }
   } continue {
      print $client "Command? ";
   }
   close $client;
 }
