#!/usr/bin/perl -w
use strict;
use warnings;
use Email::MIME;

my $message = Email::MIME->create(
                         header => [
			     From => 'titi@titi.eu',
			     To   => 'toto@toto.fr',
			           ],
                          parts => [
			      q[ This is part one],
			      q[ This is part two],
			      q[ These could be binary too],
			      ],
                         );
  # produce an Email::Abstract compatible message object,
  # e.g. produced by Email::Simple, Email::MIME, Email::Stuff

use Email::Sender::Simple qw(sendmail);
use Email::Sender::Transport::SMTP::TLS qw(all);
use Try::Tiny;
my $SMTP_ENVELOPE_FROM_ADDRESS='titi@titi.eu';
my $SMTP_HOSTNAME='smtp.titi.eu';
my $SMTP_PORT=587;
my $SMTP_SSL=1;
my $SMTP_SASL_USERNAME='titi';
my $SMTP_SASL_PASSWORD='titi';

my $transport = Email::Sender::Transport::SMTP::TLS->new(
        host => $SMTP_HOSTNAME,
        port => 587,
        username => $SMTP_SASL_USERNAME,
        password => $SMTP_SASL_PASSWORD,
        helo => $SMTP_HOSTNAME,
    );

try {
        sendmail($message, { transport => $transport });
} catch {
        die "Error sending email: $_";
};
