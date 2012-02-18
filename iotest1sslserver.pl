#!/usr/bin/perl


use strict;
use warnings;

use IO::Socket::SSL 'inet4';
use IO::Select       qw( );


sub process_msg {
    my ($client, $msg) = @_;
    chomp $msg;
    my $host = $client->peerhost;
    print "$host said '$msg'\n";
    return lc($msg) eq 'quit';
}


my $server = IO::Socket::SSL->new(

LocalPort => 42011,
                                 Proto     => 'tcp',
                                 Reuse     => 1,
                                 Listen    => 10,
                                 SSL_use_cert => 1,
                                 SSL_verify_mode => 0x00,
                                 SSL_key_file    => '/home/swilting/perltest/private/localhost.key',
                                 SSL_cert_file   => '/home/swilting/perltest/certs/localhost.cert',
                                 SSL_passwd_cb => sub { return "" },

) or die("Couldn create server socket:$!\n");

my $select = IO::Select->new($server);

my %bufs;

while (my @ready = $select->can_read) {
    for my $fh (@ready) {
        if ($fh == $server) {
            my $client = $server->accept;
            $select->add($client);
            $bufs{fileno($client)} = '';

            my $host = $client->peerhost;
            print "[Accepted connection from $host]\n";
        }
        else {
            our $buf; local *buf = \$bufs{fileno($fh)};
            my $rv = sysread($fh, $buf, 64*1024);
            if (!$rv) {
                my $host = $fh->peerhost;
                if (defined($rv)) {
                    print "[Connection from $host terminated]\n";
                } else {
                    print "[Error reading from host $host: $!]\n";
                }
                process_msg($fh, $buf) if length($buf);
##                my $select->delete($bufs{fileno($fh)});
##                my $select->delete($fh);
	        $bufs{fileno($fh)} = '';
                next;
            }

            while ($buf =~ s/\G(.*\n)//g) {
                if (!process_msg($fh, "$1")) {
                    my $host = $fh->peerhost;
                    print "[Connection from $host terminated]\n";
##                    my $select->delete($bufs{fileno($fh)});
##                    my $select->delete($fh);
                    $bufs{fileno($fh)} = '';
                    last;
		    }
		}
	    }
	}
    }
__END__
