#####################################################
#!/usr/bin/perl
use warnings;
use strict;
use Net::EasyTCP;
use Tk;
use Tk::ROText;
use IO::Select;

# create the socket

my $user = 'user1';
my $pass = 1;

my $host = "localhost";
my $port = "2345";
my $portpassword = "tkscrabble";
my %hash;     #global used for sending data  
my $sendmode = 0;  #flag to switch between send and receive mode 

my $client = new Net::EasyTCP(
mode            =>      "client",
host            =>      'localhost',
port            =>      $port,
password        =>      $portpassword
) || die "ERROR CREATING CLIENT: $@\n";

my $encrypt = $client->encryption();
my $compress = $client->compression();

my $socket = $client->socket();
my $sel    = new IO::Select($socket);

my $reply;

my $mw  = new MainWindow(-background => 'lightsteelblue');
my $rotext = $mw->Scrolled('ROText', -scrollbars=>'ose',
                           -background => 'black',
                           -foreground  => 'lightyellow',    
                         )->pack;

&displayit($rotext ,"encryption method ->$encrypt\ncompression method
->$compress\n");

my $ent = $mw->Entry()->pack(qw/-fill x -pady 5/);

$mw->Button(-text =>'Disconnect', 
            -command =>sub{$client->close();Tk::exit },
        -background => 'pink',
        -activebackground => 'hotpink',
           )->pack();

$mw ->bind('<Any-Enter>' => sub { $ent->Tk::focus });
$ent->bind('<Return>' => [\&broadcast, $client]);

$reply = $client->receive() || die "ERROR RECEIVING: $@\n";
if($reply eq 'LOGIN'){ &logon }

$reply = $client->receive() || die "ERROR RECEIVING: $@\n";

$mw->repeat(11, sub {
    while($sel->can_read(.01)) {
    my $reply = $client->receive();
         &displayit($rotext, $reply);
        }
    }); 

                    
MainLoop;
##############################################################
sub broadcast {
    my ($ent, $client) = @_;

    my $text = $ent->get;
    $ent->delete(qw/0 end/);

    $client->send($text);
}
##############################################################
sub displayit {
   my ($rotext,$data) = @_;
  
  $rotext->insert('end',$data);
  $rotext->see('end');
}

###############################################################
sub logon{

# this section checks user and password
# it exits when an OK-SEND-> is received
while(1){
$hash{'user'} = $user;
$hash{'pass'} = $pass;

my $reply = &sendhash();
print "$reply\n";
if($reply =~ /^OK-SEND->(.*)/){last}
}

}
###############################################################
#################################################################
sub sendhash{
$client->send(\%hash) || die "ERROR SENDING: $@\n";
my $reply = $client->receive() || die "ERROR RECEIVING: $@\n";
return $reply;
}
################################################################ 
__END__
