#!/usr/local/bin/perl
#
# Perl Chat Server modified for une ssl socket
# Author: Torrance Jones <torrancejones@users.sourceforge.net>
# Date: August 23, 2000
# License: GNU GPL http://www.fsf.org/copyleft/gpl.html
#    Copyright (C) 2000 Torrance Jones
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
#    USA
#
# Version: 0.1.1 (Alpha)
# Other Contributors: No Official Developers Have yet to Contribute.
#
#                     However, lots of the multiplexing code was borrowed
#                     and modified from Oreilly's "Perl Cookbook" written
#                     by Tom Christiansen and Nathan Torkington.  Recipe 17.13
#
# Changes: 
#    0.1.0 rewrite of socket level code... :(
#    0.0.8 fixed some incorrect messages that were being printed
#             to the client
#          added query_buddy option
#    0.0.7 added Global Message support
#          print license at server startup
#    0.0.6 changed the format of msg_users() again
#    0.0.5 Added the "from" field when sending a message to a user
#          finished msg_user() sub
#          wrote get_link() to search logged users for the correct
#            connection/socket to send a msg to
#    0.0.4 updated format of server replies
#    0.0.3 changed format of data structure %hosts
#          add_remote_host() and chk_for_connections() diabled
#          added dumphash(0 to aid in debugging
#          modified the usage of %hosts to block multiple consecutive
#            logins from the same host.
#          altered login() quit() logout() to call chk_for_login()
#            differently
#          altered chk_for_login() to utilize %hosts
#          altered rm_user() to delete a key from the hash
#
# SERVER/ERROR MESSAGES:
#    00 - Not Implemented Yet
#    01 - Not Logged Off
#    02 - User Already Exists
#    03 - Logged In Successful
#    04 - Could not Create New User
#    05 - Unrecognized ID/Server Protocol
#    06 - Successfully Registered
#    07 - Successfully Terminated/Logged Off
#    08 - Password File Problems
#    09 - Illegal null field
#    10 - Illegal Logoff Attempted
#    11 - Quit Without Login
#    12 - User Already Logged In
#    13 - User Not Logged In
#    14 - User Logged Off From Client
#    15 - User Logged Off From Server
#    16 - Duplicate Connect Attempted
#    17 - Get Logged Users
#    18 - Illegal Data Grab
#    19 - Illegal Message Attempt
#    20 - Message Sent
#    21 - Global Message Sent
#    22 - User query
#    23 - Target Not Logged In
#
# SERVER PROTOCOLS
#    01 - Login                 ....... 1::username::passwd
#    02 - Register New User             2::username::passwd
#    03 - Terminate Program     ....... 3::username::passwd
#    04 - Logoff Server                 4::username::passwd
#    05 - Get Logged Users      ....... 5::username::passwd
#    06 - Message another user          6::username::passwd::rcpt::message
#    07 - Global message        ....... 7::username::passwd::message
#    08 - Query buddy                   8::username::passwd::rcpt
#    09 - Update Buddy Info     ....... 9::username::passwd::Name::Email::Quote
#
# TODO: check for stale users that no longer exist
#       update clients when new user logs in/out
#       Block users from registering multiple names in a certian
#          time period.
#       Block multiple logins from the same host
#       Pass @arry to return_logged_users() and replace 6 from sends
#------------------------------------------------------------

use POSIX;
use IO::Socket::SSL 'inet4';
use IO::Select;
use Socket;
use Fcntl;
use Tie::RefHash;

$port = 42017;               # change this at will
my %hosts;
my $VERSION = "0.1.1";
my $DATE = "August 23, 2000";

print " Perl Chat Server v$VERSION
 modified for use ssl socket
 http://perlchat.sourceforge.net
 Author: Torrance Jones <torrancejones\@users.sourceforge.net>
 Date Last Modified: $DATE
 License: GNU GPL http://www.fsf.org/copyleft/gpl.html
    Copyright (C) 2000 Torrance Jones

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
    USA\n\n";

# Create Server!
$server = IO::Socket::SSL->new(LocalPort => $port,
                                Listen    => 100, 
                                SSL_use_cert => 1,
                                SSL_verify_mode => 0x04,
                                SSL_key_file => '/home/swilting/perltest/private/ks37777.kimsufi.com.key',
                                SSL_cert_file => '/home/swilting/perltest/certs/ks37777.kimsufi.com.cert',
				SSL_ca_file   => '/home/swilting/perltest/certs/ks37777.kimsufi.com.cert',
                                SSL_passwd_cb => sub { return "" },
                                 )
  or die "Can't make server socket: $@\n";

print "Server created. Waiting for events...\n";

# begin with empty buffers
%inbuffer  = ();
%outbuffer = ();
%ready     = ();

tie %ready, 'Tie::RefHash';

nonblock($server);
$select = IO::Select->new($server);

# Main loop: check reads/accepts, check writes, check ready to process
while (1) {
    my $client;
    my $rv;
    my $data;

    # check for new information on the connections we have

    # anything to read or accept?
    foreach $client ($select->can_read(1)) {

        if ($client == $server) {
            # accept a new connection
            $client = $server->accept();
            $select->add($client);
            nonblock($client);
        } else {
            # read data
            $data = '';
            $rv   = $client->sysread($data, POSIX::BUFSIZ, 0);

            unless (defined($rv) && length $data) {
                # This would be the end of file, so close the client
                delete $inbuffer{$client};
                delete $outbuffer{$client};
                delete $ready{$client};

                $select->remove($client);
                close $client;
                next;
            }

            $inbuffer{$client} .= $data;

            # test whether the data in the buffer or the data we
            # just read means there is a complete request waiting
            # to be fulfilled.  If there is, set $ready{$client}
            # to the requests waiting to be fulfilled.
            while ($inbuffer{$client} =~ s/(.*\n)//) {
                push( @{$ready{$client}}, $1 );
            }
        }
    }

    # Any complete requests to process?
    foreach $client (keys %ready) {
        handle($client);
    }

    # Buffers to flush?
    foreach $client ($select->can_write(1)) {
        # Skip this client if we have nothing to say
        next unless exists $outbuffer{$client};

        $rv = $client->syswrite($outbuffer{$client}, 0);
        unless (defined $rv) {
            # Whine, but move on.
            warn "I was told I could write, but I can't.\n";
            next;
        }
        if ($rv == length $outbuffer{$client} ||
            $! == POSIX::EWOULDBLOCK) {
            substr($outbuffer{$client}, 0, $rv) = '';
            delete $outbuffer{$client} unless length $outbuffer{$client};
        } else {
            # Couldn't write all the data, and it wasn't because
            # it would have blocked.  Shutdown and move on.
            delete $inbuffer{$client};
            delete $outbuffer{$client};
            delete $ready{$client};

            $select->remove($client);
            close($client);
            next;
        }
    }

    # Out of band data?
    foreach $client ($select->has_exception(0)) {  # arg is timeout
        # Deal with out-of-band data here, if you want to.
        print "DEBUG ME!\n";
    }
}

# handle($socket) deals with all pending requests for $client
sub handle {
    # requests are in $ready{$client}
    # send output to $outbuffer{$client}
    my $client = shift;
    my $request;

    foreach $request (@{$ready{$client}}) {
        # $request is the text of the request
        # put text of reply into $outbuffer{$client}
        chomp $request;
        rcvd_msg_from_client($client, $request);

#        $outbuffer{$client} .= "$request";
    }
    delete $ready{$client};
}
#-----------------------------------------------------------------------------
# nonblock($socket) puts socket into nonblocking mode
sub nonblock {
    my $socket = shift;
    my $flags;

    
    $flags = fcntl($socket, F_GETFL, 0)
            or die "Can't get flags for socket: $!\n";
    fcntl($socket, F_SETFL, $flags | O_NONBLOCK)
            or die "Can't make socket nonblocking: $!\n";
}
#----------------------------------------------------------------------------
sub rcvd_msg_from_client {
# This sub receives the message from the client and processes
# it.
# Preconditions: socket must be established, should have
#                checked for multiple sockets to this server
#                which originate from the same remote
#                location
# Postconditions: ?
#
    my ($client, $request) = @_;

    if (length($request) ne 0) {
        chomp $request;
        print "CLIENT QUERY: '$request'\n";
        # CLIENT QUERY in the form of ID_NUMBER::USERNAME::PASSWD:: ...
        my @msg = split(/::/, $request);
        if($msg[0] eq '1') {
           logon($client, @msg);
        } elsif ($msg[0] eq 2) {
           register($client, @msg);
        } elsif ($msg[0] eq 3) {
           quit($client, @msg);
        } elsif ($msg[0] eq 4) {
           logoff($client, @msg);
        } elsif ($msg[0] eq 5) {
           return_logged_users($client, @msg);
        } elsif ($msg[0] eq 6) {
           msg_user($client, @msg);
        } elsif ($msg[0] eq 7) {
           msg_all_users($client, @msg);
        } elsif ($msg[0] eq 8) {
           query_buddy($client, @msg);
        } elsif ($msg[0] eq 9) {
           update_info($client, @msg);
        } else {
           print "Unrecognized ID $msg[0]\n";
           $outbuffer{$client} .= "Unrecognized ID $msg[0]\n";   
        }
    }
}
#-----------------------------------------------------------------------------
sub logon {
# This sub checks the length of the data passed to it,
# if the length is ne 0, then it checks to make sure the
# data authenticates (i.e. login name and password authenticate)
# If this happens then chk_for_login is called else user is
# logged in.
#
# Pre-Conditions:
# Post-Conditions: If user is not already logged in, that user
#                  is added to the hash %hosts
#                  Message is printed to both server and client
#
   my ($client, @msg) = @_;

   my $client_ip = get_hostaddr($client);

   if (length($msg[1]) ne 0 || length($msg[2]) ne 0) {
      print "User $msg[1] attempting login...\n";
      if (authorize($msg[1], $msg[2])) {
        # check if user is already logged in
        if (chk_for_login($msg[1])) {    # check if username is already in use
          # user is already logged in
          print "SERVER::",time_stamp(-4),
                "::03::User $msg[1] $client_ip logged in.\n";
          $outbuffer{$client} .= "SERVER::$msg[0]::03::$msg[1] logged in!\n";

        } else {
          # user is not logged in, add to %hosts
          my $current_time = time_stamp(-4);
          $hosts{$msg[1]}->{'ip'} = $client_ip;
          $hosts{$msg[1]}->{'status'} = 'connected';
          $hosts{$msg[1]}->{'logged_in'} = 'yes';
          $hosts{$msg[1]}->{'user_name'} = $msg[1];
          $hosts{$msg[1]}->{'password'} = $msg[2];
          $hosts{$msg[1]}->{'con_time'} = $current_time;
          $hosts{$msg[1]}->{'connection'} = $client;
          $hosts{$msg[1]}->{'real_name'} = 'nil';
          $hosts{$msg[1]}->{'email'} = 'nil';
          $hosts{$msg[1]}->{'quote'} = 'nil';
          print "SERVER::",time_stamp(-4),"::03::User $msg[1] $client_ip logged in.\n";
          $outbuffer{$client} .= "SERVER::$msg[0]::03::$msg[1] logged in!\n";
        }
      } else {
         print "SERVER::",time_stamp(-4),"::00::User $msg[1] NOT logged in.\n";
         $outbuffer{$client} .= "SERVER::$msg[0]::00::$msg[1] NOT logged in!\n";
      }
   } else {
      print "SERVER::",time_stamp(-4),"::00::Null user not logged in.\n";
      print "ERROR::",time_stamp(-4),"::09::Invalid logon attempt.\n";
      $outbuffer{$client} .= "SERVER::$msg[0]::00::Could not login with that name.\n";
   }
}
#-----------------------------------------------------------------------------
sub get_hostaddr {
# Preconditions: $client must be established and defined
# Postconditions: the peerhost of $conn will be returned
   my ($client) = @_;
   my $sock = $client;
   return $sock->peerhost();
}
#-----------------------------------------------------------------------------
sub time_stamp {
# This sub returns the time using gmtime(); This sub requires
# that one argument be passed to it, the time zone differnece
# from gmt, for eastern time it would be '-4', pacific would
# be '-7', etc...
#
# Preconditions: calling parameter must be timezone offset
# Postconditions: the time and date is returned
my ($time_zone_offset) = @_;
my ($s, $m, $h, $dy, $mo, $yr, $wd, $dst);
   ($s, $m, $h, $dy, $mo, $yr, $wd, $dst) = gmtime(); # get the date
   $mo++;
   $yr = $yr - 100;
   if ($mo < 10) { $mo = "0".$mo; }
   if ($dy < 10) { $dy = "0".$dy; }
   if ($yr < 10) { $yr = "0".$yr; }
   if ($h < 10) { $h = "0".$h; }
   if ($m < 10) { $m = "0".$m; }
   if ($s < 10) { $s = "0".$s; }
   $h = $h + $time_zone_offset; # adjust for time zones
   return "$mo-$dy-$yr $h:$m:$s";
}
#-----------------------------------------------------------------------------
sub authorize {
# Returns 0 if user is found in password file else returns 1.
# Preconditions: password file must exist
# Postconditions: returns either 0 or 1
#
# todo: make the name of the passwd file a variable
#       maybe make this faster, e.g. merge with checkforuser()
#           search for user then authenticate instead of
#           trying to match every user and every password
#
    my ($USER, $PASS) = @_;

    unless (open (PASSWD, "passwd")) {
       print "ERROR::",time_stamp(-4),"::08::Cannot open password file.\n";
       return 0; # ERROR
    }

    while(<PASSWD>) {
       my @passwd_arry = split (/::/);
       chomp $passwd_arry[1];
       if ($passwd_arry[0] eq $USER && $passwd_arry[1] eq $PASS) {
          close(PASSWD);
          return 1;
       }
    }
    print "ERROR::",time_stamp(-4),"::05::User $USER is NOT authenticated in password file.\n";
    close(PASSWD);
    return 0;              # ERROR
}
#-----------------------------------------------------------------------------
sub chk_for_login {
# chk_for_login() checks the hash of logins for a matching
# key.  if one is found then 1 is returned, else 0 is returned
# Preconditions: $remote_host must be a valid hostname
#                %hosts must be global
# Postconditions: 0 or 1 is returned [1 = matched 0 = nomatched]
#
my ($user_name) = @_;
foreach (keys %hosts) {
   if($_ eq $user_name) {
      return 1;
   }
}
# DANGER: Doing a test such as the one below will corrupt the data
#         structure %hosts
#   if ($hosts{$user_name}->{'logged_in'} eq 'yes') {
#      return 1;
#   }
return 0;
}
#-----------------------------------------------------------------------------
sub register {
  my ($client, @msg) = @_;
  if (length($msg[1]) ne 0 || length($msg[2]) ne 0) {
     if (checkforuser($msg[1])) {
        print "ERROR::",time_stamp(-4),"::02::User $msg[1] already exists.\n";
        $outbuffer{$client} .= "SERVER::$msg[0]::02::User $msg[1] already exists!\n";
     } else {
        # user not exist
        if (registeruser($msg[1], $msg[2])) {
           print "SERVER::",
                  time_stamp(-4),
                  "::06::User $msg[1] successfully added.\n";
           $outbuffer{$client} .= "SERVER::$msg[0]::06::Successfully added $msg[1]\n";
        } else {
           print "ERROR::",
                 time_stamp(-4),
                 "::04::user not created or passwd file probs.\n";
           $outbuffer{$client} .= "SERVER::$msg[0]::04::unknown problem registering\n";
        }
     }
  } else {
     print "SERVER::",time_stamp(-4),"::09::Logon info was null.\n";
     print "ERROR::",time_stamp(-4),"::09::Invalid logon attempt\n";
     $outbuffer{$client} .= "SERVER::$msg[0]::04::Could not create new User.\n";
  }
}
#----------------------------------------------------------------------------
sub registeruser {
# registeruser() - Add an entry to the password file
# Preconditions: password file must exist
# Postconditions: if no errors then entry is made in file.  also
#                 0 or 1 is returned depending on the success/failure
#                 of this sub.
#
   my ($USER, $PASS) = @_;
   unless(open (PASSWD, ">>passwd")) {
      print "ERROR::",time_stamp(-4),"::08::Cannot open password file.\n";
      return 0;
   }
   print PASSWD $USER,"::",$PASS,"\n";
   return 1;
}
#----------------------------------------------------------------------------
sub checkforuser {
# checkforuser() - Returns a 1 if the user is found and a 0 otherwise
# Preconditions: password file must exist
# Postcondition: either 0 or 1 is returned
#
    my ($USER) = @_;
    unless (open (PASSWD, "passwd")) {
       print "ERROR::",time_stamp(-4),"::08::Cannot open password file.\n";
       return 0; # ERROR
    }
    while(<PASSWD>) {
       my @passwd_arry = split (/::/);
       chomp $passwd_arry[1];
       if ($USER eq $passwd_arry[0]) {
          close(PASSWD);
          return 1;                      # success found user
       }
    }
    close(PASSWD);
    return 0;                            # failure user not found
}
#----------------------------------------------------------------------------
sub quit {
  my ($client, @msg) = @_;
  if (length($msg[1]) ne 0 || length($msg[2]) ne 0) {
     if (authorize($msg[1], $msg[2])) {
        # check for loggon
        if (chk_for_login($msg[1])) {
           # user is logged in, remove him!
           rm_user($msg[1]);
           print "SERVER::",time_stamp(-4),"::15::Auto Logoff User $msg[1].\n";
           $outbuffer{$client} .= "SERVER::$msg[0]::15:: Auto Loggoff GoodBye!\n";
        } else {
           # user is not logged in, ok to quit!
           print "SERVER::",time_stamp(-4),"::14::User $msg[1] Clean Quit.\n";
           $outbuffer{$client} .= "SERVER::$msg[0]::14:: Clean Quit GoodBye!\n";
        }
     } else {
        print "ERROR::",time_stamp(-4),
              "::10::Illegal quit attempt: User $msg[1]\n";
        $outbuffer{$client} .= "SERVER::$msg[0]::10::$msg[1] not logged off!\n";
     }
  } else {
      print "SERVER::",time_stamp(-4),"::11::Quit info was null.\n";
      print "SERVER::",time_stamp(-4),"::09::Quit info was null.\n";
      $outbuffer{$client} .= "SERVER::$msg[0]::07::Goodbye!\n";
  }
}
#-----------------------------------------------------------------------------
sub rm_user {
# This sub removes an element from %hosts
# PreConditions: user must have been authorized,
#                %hosts must be accessible
#                should not be two users logged in from the same host
# PostConditions: user deleted from %hosts
   my ($user_name) = @_;
   foreach (keys %hosts) {
     if ($user_name eq $_) {
       delete $hosts{$_};
       print "DEBUG::",time_stamp(-4),
             "::XX::removed $user_name from logged hash.\n";
     }
   }
}

#-----------------------------------------------------------------------------
sub logoff {
  my ($client, @msg) = @_;
  if (length($msg[1]) ne 0 || length($msg[2]) ne 0) {
     if (authorize($msg[1], $msg[2])) {
        # check for loggon
        if (chk_for_login($msg[1])) {
           # user is logged in, remove him!
           rm_user($msg[1]);
           print "SERVER::",time_stamp(-4),"::14::User $msg[1] Logoff.\n";
           $outbuffer{$client} .= "SERVER::$msg[0]::14::$msg[1] Logged Off!\n";
        } else {
           # user is not logged in, error message!
           print "SERVER::",time_stamp(-4),
                 "::13::User $msg[1] not logged in.\n";
           $outbuffer{$client} .= "SERVER::$msg[0]::13::$msg[1] not logged in!\n";
        }
     } else {
        print "ERROR::",time_stamp(-4),
              "::10::Illegal logoff attempt: User $msg[1]\n";
        $outbuffer{$client} .= "SERVER::$msg[0]::10::$msg[1] not logged off!\n";
     }
   } else {
     print "SERVER::",time_stamp(-4),"::09::Illegal null field\n";
     $outbuffer{$client} .= "SERVER::$msg[0]::11::Quit without login.\n";
  }
}
#-----------------------------------------------------------------------------
sub return_logged_users {
# This sub returns a list of the logged users to the client
# Preconditions: the client should have
#                $hosts{}->{'status'} = 'connected' and thus should
#                have been authenticated for proper username and
#                password.
#
# Postconditions: a list of users is returned
#
   my ($client, @msg) = @_;
   my $list;
   my $remote_ip = get_hostaddr($client);
   if(chk_for_login($msg[1])) {
      foreach (keys %hosts) {
        $list .= $_." ";
      }
      print "SERVER::",time_stamp(-4),"::17::$msg[1] get logged users.\n";
      $outbuffer{$client} .= "SERVER::$msg[0]::17::$list\n";
   } else {
   # Illegal attempt to get info from the server
     print "SERVER::",time_stamp(-4),"::18::Illegal data grab from $remote_ip.\n";
     $outbuffer{$client} .= "SERVER::$msg[0]::18::Please Login First!\n";
   }
}
#-----------------------------------------------------------------------------
sub msg_user {
# Pre-conditions:
# Post-conditions:
#
# @msg contains something like this for info:
#    6::user::password::recipient::message
#
   my ($client, @msg) = @_;

   if (chk_for_login($msg[1])) {    # check if user is logged in
      if(chk_for_login($msg[3])) {  # check if recipient is logged in
         my $rcpt = get_link($msg[3]); # get rcpts connection
         print "SERVER::", time_stamp(-4),
               "::20::Message sent from $msg[1] to $msg[3]\n";
         # send info to client in this format
         # SERVER::6::20::from::message
         $outbuffer{$rcpt} .= "SERVER::$msg[0]::20::$msg[1]::$msg[4]\n";

      } else {
         print "ERROR::13::Recipient $msg[3] not logged in.\n";
         $outbuffer{$client} .= "SERVER::$msg[0]::23::$msg[3] not logged in.\n";
      }
   } else {
    print "ERROR::19::Message Attempt without login\n";
    $outbuffer{$client} .= "SERVER::$msg[0]::13::Please Login First!\n";
   }
}
#-----------------------------------------------------------------------------
sub get_link {
# This sub gets a connection link to a user
# Postconditions: returns the socket connection to a logged in user
    my ($user_name) = @_;
    foreach (keys %hosts) {
       if($_ eq $user_name) {
         return $hosts{$_}->{'connection'};
       }
    }
}
#-----------------------------------------------------------------------------
sub msg_all_users {
# Preconditions: must be logged in
# Postconditions: sends a global message to every user logged in
#    proto will look like this
#    7::user::password::message
   my ($client, @msg) = @_;
   my $rcpt;

    print "SERVER::",time_stamp(-4),"::21::Global message attempt: $msg[1]\n";
   if (chk_for_login($msg[1])) {      # check if user is logged in
      print "SERVER::",time_stamp(-4),"::21::$msg[1] cleared for GM\n";
      foreach (keys %hosts) {          # for each logged in user
        $rcpt = get_link($_);    # get rcpts connection
        $outbuffer{$rcpt} .= "SERVER::$msg[0]::21::$msg[1]::$msg[3]\n";
      }
   }
}
#-----------------------------------------------------------------------------
sub query_buddy {
# Preconditions: must be logged in
# Postconditions: returns a buddies info that resides on the server

   my ($client, @msg) = @_;

   if (chk_for_login($msg[1])) {    # check if user is logged in
      if(chk_for_login($msg[3])) {  # check if buddy is logged in
         print "SERVER::", time_stamp(-4),
               "::22::$msg[1] queries $msg[3]\n";

         my $bname = $hosts{$msg[3]}->{'real_name'};
         my $bemail = $hosts{$msg[3]}->{'email'};
         my $bquote = $hosts{$msg[3]}->{'quote'};
         my $bip = $hosts{$msg[3]}->{'ip'};
         my $blogged_in = $hosts{$msg[3]}->{'logged_in'};
         my $bstatus = $hosts{$msg[3]}->{'status'};
         my $bcon_time = $hosts{$msg[3]}->{'con_time'};

         my $totalinfo = $bname."::".$bemail."::".$bquote."::".$bip."::".$blogged_in."::".$bstatus."::".$bcon_time;

         $outbuffer{$client} .= "SERVER::$msg[0]::22::$msg[3]::$totalinfo\n";
      } else {
         print "ERROR::23::Recipient $msg[3] not logged in.\n";
         $outbuffer{$client} .= "SERVER::$msg[0]::23::$msg[3] not logged in.\n";
      }
   } else {
      print "ERROR::19::Query Attempt without login\n";
      $outbuffer{$client} .= "SERVER::$msg[0]::13::Please Login First!\n";
   }
}#----------------------------------------------------------------------------
sub update_info {
# Preconditions: must be logged in
# Postconditions: updates a buddies info
# format will look like this --> 9::user::pass::name::email::quote

   my ($client, @msg) = @_;
   
print "DEBUG $msg[3] $msg[4] $msg[5] \n";

   if (chk_for_login($msg[1])) {
      $hosts{$msg[1]}->{'real_name'} = $msg[3];
      $hosts{$msg[1]}->{'email'} = $msg[4];
      $hosts{$msg[1]}->{'quote'} = $msg[5];
   }



}
