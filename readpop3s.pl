#!/usr/bin/perl -w
use strict;
use warnings;
use Mail::POP3Client;
my$pop = new Mail::POP3Client( USER     => 'john.swilting',
                                 PASSWORD => '*****',
                                 HOST     => 'pop.wanadoo.fr',
                                 USESSL   => 1, );
for( my$i = 1; $i <= $pop->Count(); $i++ ) {
    foreach( $pop->Head( $i ) ) {
       /^(From│Subject):\s+/i && print $_, "\n";
       }
    }
$pop->Close();
