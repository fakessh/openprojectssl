#!/usr/bin/perl -w

use Mail::POP3Client;
$pop = new Mail::POP3Client( USER     => 'john.swilting',
                                 PASSWORD => '*****',
                                 HOST     => 'pop.wanadoo.fr',
                                 USESSL   => 1, );
for( $i = 1; $i <= $pop->Count(); $i++ ) {
    foreach( $pop->Head( $i ) ) {
       /^(From│Subject):\s+/i && print $_, "\n";
       }
    }
$pop->Close();
