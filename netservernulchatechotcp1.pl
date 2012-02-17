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
               SSL_cert_file => '/home/swilting/perltest/private/localhost.key',
               SSL_key_file  => '/home/swilting/perltest/certs/localhost.cert',
	       port          => '42000',
              );
__PACKAGE__->run();
exit;

package Server::TchatSSL::CGI;

use base 'HTTP::Server::Simple::CGI';
use strict;
use base qw(Net::Server::Multiplex);
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
package SampleChatServer;

use strict;
use base qw(Net::Server::Multiplex);

# Demonstrate a Net::Server style hook
sub allow_deny_hook {
  my $self = shift;
  my $prop = $self->{server};
  my $sock = $prop->{client};

  return 1 if $prop->{peeraddr} =~ /^127\./;
  return 0;
}


# Another Net::Server style hook
sub request_denied_hook {
  print "Go away!\n";
  print STDERR "DEBUG: Client denied!\n";
}


# IO::Multiplex style callback hook
sub mux_connection {
  my $self = shift;
  my $mux  = shift;
  my $fh   = shift;
  my $peer = $self->{peeraddr};
  # Net::Server stores a connection counter in the {requests} field.
  $self->{id} = $self->{net_server}->{server}->{requests};
  # Keep some values that I might need while the {server}
  # property hash still contains the current client info
  # and stash them in my own object hash.
  $self->{peerport} = $self->{net_server}->{server}->{peerport};
  # Net::Server directs STDERR to the log_file
  print STDERR "DEBUG: Client [$peer] (id $self->{id}) just connected...\n";
  # Notify everyone that the client arrived
  $self->broadcast($mux,"JOIN: (#$self->{id}) from $peer\r\n");
  # STDOUT is tie'd to the correct IO::Multiplex handle
  print "Welcome, you are number $self->{id} to connect.\r\n";
  # Try out the timeout feature of IO::Multiplex
  $mux->set_timeout($fh, undef);
  $mux->set_timeout($fh, 20);
  # This is my state and will be unique to this connection
  $self->{state} = "junior";
}


# If this callback is ever hooked, then the mux_connection callback
# is guaranteed to have already been run once (if defined).
sub mux_input {
  my $self = shift;
  my $mux  = shift;
  my $fh   = shift;
  my $in_ref = shift;  # Scalar reference to the input
  my $peer = $self->{peeraddr};
  my $id   = $self->{id};

  print STDERR "DEBUG: input from [$peer] ready for consuming.\n";
  # Process each line in the input, leaving partial lines
  # in the input buffer
  while ($$in_ref =~ s/^(.*?)\r?\n//) {
    next unless $1;
    my $message = "[$id - $peer] $1\r\n";
    $self->broadcast($mux, $message);
    print " - sent ".(length $message)." byte message\r\n";
  }
  if ($self->{state} eq "senior") {
    $mux->set_timeout($fh, undef);
    $mux->set_timeout($fh, 40);
  }
}


# It is possible that this callback will be called even
# if mux_connection or mux_input were never called.  This
# occurs when allow_deny or allow_deny_hook fails to
# authorize the client.  The callback object will be the
# default listen object instead of a client unique object.
# However, both object should contain the $self->{net_server}
# key pointing to the original Net::Server object.
sub mux_close {
  my $self = shift;
  my $mux  = shift;
  my $fh   = shift;
  my $peer = $self->{peeraddr};
  # If mux_connection has actually been run
  if (exists $self->{id}) {
    $self->broadcast($mux,"LEFT: (#$self->{id}) from $peer\r\n");
    print STDERR "DEBUG: Client [$peer] (id $self->{id}) closed connection!\n";
  }
}


# This callback will happen when the mux->set_timeout expires.
sub mux_timeout {
  my $self = shift;
  my $mux  = shift;
  my $fh   = shift;
  print STDERR "DEBUG: HEARTBEAT!\n";
  if ($self->{state} eq "junior") {
    print "Whoa, you must have a lot of patience.  You have been upgraded.\r\n";
    $self->{state} = "senior";
  } elsif ($self->{state} eq "senior") {
    print "If you don't want to talk then you should leave. *BYE*\r\n";
    close(STDOUT);
  }
  $mux->set_timeout($fh, undef);
  $mux->set_timeout($fh, 40);
}


# Routine to send a message to all clients in a mux.
sub broadcast {
  my $self = shift;
  my $mux  = shift;
  my $msg  = shift;
  foreach my $fh ($mux->handles) {
    # NOTE: All the client unique objects can be found at
    # $mux->{_fhs}->{$fh}->{object}
    # In this example, the {id} would be
    #   $mux->{_fhs}->{$fh}->{object}->{id}
    print $fh $msg;
  }
}

1;
__END__
