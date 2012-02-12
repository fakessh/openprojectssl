####################################################
#!/usr/bin/perl
use warnings;
use strict;
use Net::EasyTCP;

$|=1;

my $debug = 0; # set to 1 for verbose server messages

local $SIG{INT} = sub { close LOG;print "Exiting\n";exit 0;};
print "Hit control-c to stop server\n";

my @nocrypt = qw(Crypt::RSA); #too slow, but more secure
#my @nocompress = qw(Compress::LZF); #just testing 

my $host = "localhost";
my $port = "2345";
my $portpassword = "tkscrabble";
my $logdir = 'logs';
my %okusers = (user1 => '1',
               user2 => '2',
               user3 => '3',
               user4 => '4',
                test => 'test',
            joe  => 'zzzz',
        );
            
my %clients;
          
if (! -d $logdir) {
         mkdir($logdir,0755) ;
         print "Log directory created: $logdir\n" ;
         }

my $logname = "port_$port-".&gettime.'.log';  #try to give a unique name
open (LOG,">>$logdir/$logname") 
       or die "Couldn't open port $port log: $!";

my $server = new Net::EasyTCP(
    host  =>  "localhost", 
    mode  =>  "server",
    port  =>   $port,
    password => $portpassword,  # if using asymmetric encryption, 
                                # port password gives better security
    donotencryptwith => \@nocrypt,
#    donotencrypt => 1,
#    donotcompresswith => \@nocompress,
#    donotcompress => 1,
    )
       || die "ERROR CREATING SERVER: $@\n";

$server->setcallback(
     data            =>      \&gotdata,
     connect         =>      \&connected,
     disconnect      =>      \&disconnected,
    )
     || die "ERROR SETTING CALLBACKS: $@\n";

$server->start() || die "ERROR STARTING SERVER: $@\n";
####################################################
sub gotdata() {

my $client = shift;
print "client->$client\n" if $debug;
my $serial = $client->serial();
my $data = $client->data();
my $reply;

#logon
if(! defined  $clients{$serial}{'username'}){
   my $user = $data->{'user'};
   print "user->$user\n" if $debug;
   my $pass = $data->{'pass'};
   print "pass->$pass\n" if $debug;

$reply = &process_user($user,$pass,$serial,$client);
  print "$reply\n" if $debug;
$client->send($reply);
}
#logged in here

print " $serial->$data\n" if $debug;
print "$clients{$serial}{'username'}->$data\n" if $debug;

foreach my $sernum( keys %clients){
   if(defined $clients{$sernum}{'socket'}){

$clients{$sernum}{'socket'}->send("$clients{$serial}{'username'}->$data\n")
                      || die "ERROR SENDING TO CLIENT: $@\n";
     }
}

if ($data eq "QUIT") {
    $client->close() || die "ERROR CLOSING CLIENT: $@\n";
  }
   elsif ($data eq "DIE") {
    $server->stop() || die "ERROR STOPPING SERVER: $@\n";
    }

}
#####################################################
sub connected() {
 my $client = shift;
 my $serial = $client->serial();
 print "Client $serial just connected\n";
 print LOG "Client $serial just connected at ",gettime(),"\n";
 $client->send('LOGIN') || die "ERROR SENDING LOGIN TO CLIENT: $@\n";
}
###################################################
sub disconnected() {
 my $client = shift;
 my $serial = $client->serial();

 print "$clients{$serial}{'username'} just disconnected\n";
 print LOG "$clients{$serial}{'username'} just disconnected at
",&gettime,"\n";

foreach my $sernum( keys %clients){
   if(defined $clients{$sernum}{'socket'}){

$clients{$sernum}{'socket'}->send("$clients{$serial}{'username'}->Has
Logged Out\n") 
                      || die "ERROR SENDING TO CLIENT: $@\n";
    }
}

delete $clients{$serial}{'username'};
delete $clients{$serial}{'socket'};
delete $clients{$serial};
}

###########################################################
sub process_user{
my ($user,$pass,$serial,$client) = @_; 

print "proceesing $user $pass\n" if $debug;
my $reply;
my $time = gettime();

if((! defined $okusers{$user}) or ($pass ne $okusers{$user}))
        { print LOG "BADPASSWORD  $user  $pass  $time\n"; LOG->flush;
      $reply = "ERROR1: user or password id bad\n";
          return $reply;
     }

$reply = "OK-SEND->$user";
print "$reply\n";
print LOG "$reply at $time\n"; LOG->flush;
$clients{$serial}{'username'} = $user;
$clients{$serial}{'socket'} = $client;
print "$reply\n" if $debug;
return $reply;
}

#####################################################
sub gettime{
 my $date_string = localtime;
 $date_string =~ tr/ /_/;
 return $date_string;
}
######################################################3
__END__
