#!/usr/bin/perl -w

use strict;
use warnings;
use CGI ":all";
use HTTP::Daemon;
use HTTP::Status;
use Chatbot::Eliza;

$|++;


my $s = Server::TchatSSL::CGI->new(@ARGV);
$s->background(
               SSL_cert_file => '/etc/ssl/apache2/server.crt',
               SSL_key_file  => '/etc/ssl/apache2/server.key',
              );

exit;

package Server::TchatSSL::CGI;

use base 'HTTP::Server::Simple::CGI';

sub net_server { 'Net::Server::PreFork' }

package HTTP::Server::Simple::CGI;


use HTTP::Server::Simple::CGI;
use base qw(HTTP::Server::Simple::CGI);

my %dispatch = (
        'hello' => \&handle_request,    
);
sub handle_request {
     my $self = shift;
     my $cgi  = shift;

     my $path = $cgi->path_info();
     my $handler = $dispatch{$path};

    if (ref($handler) eq "CODE") {
        print "HTTP/1.0 200 OK\r\n";
        $handler->($cgi);
	} else {
           print "HTTP/1.0 404 Not found\r\n";
           print $cgi->header,
           $cgi->start_html('not found'),
           $cgi->h1('not found'),
           $cgi->end_html;
       }
}

sub resp_hello {
            my $cgi  = shift;   # CGI.pm object
            return if !ref $cgi;

   my $who = $cgi->param('name');

   print $cgi->header,
                  $cgi->start_html("Hello"),
                  $cgi->h1("Hello $who!"),
                  $cgi->end_html;
}



1;
__END__
