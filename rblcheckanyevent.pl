#!perl -w
use strict;
use warnings;
use AnyEvent;
use AnyEvent::DNS;
use Net::IP;

my %results;
my $hostname = "41.215.160.191";
my $outstanding = AnyEvent->condvar;
my $ip= new Net::IP($hostname);
$_ = $ip->reverse_ip();
s/in\-addr\.arpa\.//gi;
my $reverse = $_;


foreach  (<DATA>) {
    my $domain = $reverse . $_;
    ##$domain =~ s!\s+$!!;
    
    # Fire off a query for this hostname
    warn "Querying ip(s) for '$domain'";
    $outstanding->begin();
    AnyEvent::DNS::resolver->resolve (
      $domain, "a", sub {
        $outstanding->end;
        if( @_ ) {
            for (@_) {
                my ($name, $type, $in, $ttl, $ip) = @$_;
                $results{ $name } ||= [];
                warn "Response for '$name' '$type' '$in' '$ttl' '$ip'";
                push @{$results{ $name }}, $ip;
            };
        } else {
            warn "No response for $domain";
        };
    });
};

# Wait for all requests to finish
$outstanding->recv;

use Data::Dumper;
warn Dumper \%results;

__DATA__
b.barracudacentral.org
cbl.abuseat.org
dnsbl.invaluement.com
http.dnsbl.sorbs.net
misc.dnsbl.sorbs.net
socks.dnsbl.sorbs.net
web.dnsbl.sorbs.net
dnsbl-1.uceprotect.net
dnsbl-3.uceprotect.net
sbl.spamhaus.org
zen.spamhaus.org
psbl.surriel.com
dnsbl.njabl.org
rbl.spamlab.com
ircbl.ahbl.org
noptr.spamrats.com
cbl.anti-spam.org.cn
dnsbl.inps.de
httpbl.abuse.ch
korea.services.net
virus.rbl.jp
wormrbl.imp.ch
rbl.suresupport.com
ips.backscatterer.org
opm.tornevall.org
multi.surbl.org
tor.dan.me.uk
relays.mail-abuse.org
rbl-plus.mail-abuse.org
access.redhawk.org
rbl.interserver.net
bogons.cymru.com
bl.spamcop.net
dnsbl.sorbs.net
dul.dnsbl.sorbs.net
smtp.dnsbl.sorbs.net
spam.dnsbl.sorbs.net
zombie.dnsbl.sorbs.net
dnsbl-2.uceprotect.net
pbl.spamhaus.org
xbl.spamhaus.org
bl.spamcannibal.org
ubl.unsubscore.com
combined.njabl.org
dnsbl.ahbl.org
dyna.spamrats.com
spam.spamrats.com
cdl.anti-spam.org.cn
drone.abuse.chdul.ru
short.rbl.jp
spamrbl.imp.ch
virbl.bit.nl
dsn.rfc-ignorant.org
dsn.rfc-ignorant.org
netblock.pedantic.org
ix.dnsbl.manitu.net
rbl.efnetrbl.org
blackholes.mail-abuse.org
dnsbl.dronebl.org
db.wpbl.info
query.senderbase.org
