#!/usr/bin/perl 
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
use Email::Sender::Transport::SMTP qw(all);
use Try::Tiny;
my $SMTP_ENVELOPE_FROM_ADDRESS='titi@titi.eu';
my $SMTP_HOSTNAME='smtp.titi.eu';
my $SMTP_PORT=465;
my $SMTP_SSL=1;
my $SMTP_SASL_USERNAME='titi';
my $SMTP_SASL_PASSWORD='titi';
  try {
    sendmail(
      $message,
      {
        from => $SMTP_ENVELOPE_FROM_ADDRESS,
        transport => Email::Sender::Transport::SMTP->new({
            host => $SMTP_HOSTNAME,
            port => $SMTP_PORT,
            ssl  => $SMTP_SSL,
            sasl_username => $SMTP_SASL_USERNAME,
            sasl_password => $SMTP_SASL_PASSWORD,
        })
      }
    );
  } catch {
      warn "sending failed: $_";
  };
