#!/usr/bin/perl
#Usage:
#wget.pl "<URL(s)>" [wait]
die("Please provide one or more URLs.\n") unless ($ARGV[0]);
my $waitTime = 1;
$waitTime = $ARGV[1] if ($ARGV[1]);
system("wget -mpcki --user-agent=\"\" -e robots=off --wait $waitTime $ARGV[0]");
