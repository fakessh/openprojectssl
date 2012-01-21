#!/usr/bin/perl
 
# see http://perlmonks.org/?node_id=890441
 
use strict;
use warnings;
 
use Net::OpenSSH;
use Expect;
$Expect::Exp_Internal = 1;
  
@ARGV == 2 or die <<EOU;
Usage:
  $0 host user_passwd
 
EOU
 
my $host = $ARGV[0];
my $pass1 = $ARGV[1];
 
my $ssh = Net::OpenSSH->new($host, passwd => $pass1);
$ssh->error and die "unable to connect to remote host: " . $ssh->error;
 
#$ssh->system("sudo -k");
 
my ( $pty, $pid ) = $ssh->open2pty({stderr_to_stdout => 1}, '/usr/local/bin/sudo', -p => 'runasroot:', 'su', '-')
    or return "failed to attempt su: $!\n";
 
my $expect = Expect->init($pty);
$expect->log_file("expect.pm_log", "w"); 
my @cmdlist =("ls -l","pwd","ls","who am i","id","whoami");
foreach my $cmd (@cmdlist){
$expect->expect(2,
                [ qr/runasroot:/ => sub { shift->send("$pass1\n");} ],  #use pass2 if using only su
                [ qr/Sorry/       => sub { die "Login failed" } ],
                [ qr/#/ => sub { shift->send("$cmd \n");}]
                ) or die "___Timeout!";
}
$expect->expect(2);