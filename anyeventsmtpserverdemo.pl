#!/usr/bin/perl -w

use AnyEvent::SMTP::Server;
use strict;
use warnings;
my $good;

my $server = AnyEvent::SMTP::Server->new(
    port => 25,
    mail_validate => sub {
	my ($m,$addr) = @_;
	if ($good) { return 1 } else { return 0 , 513 , 'Bad sender.'}
    },
    rcpt_validate => sub {
	my ($m,$addr) = @_;
	if ($good) { return 1 } else { return 0 , 513 , 'Bad recipient.'}
    },
);

$server->reg_cb(
    client => sub {
	my ($s,$con) = @_;
	warn "Client from $con->{host}:$con->{port} connected\n";
    },
    disconnect => sub {
	my ($s,$con) = @_;
	warn "Client from $con->{host}:$con->{port} gone\n";
    },
    mail => sub {
	my ($s,$mail) = @_;
	warn "Received mail from $mail->{from} to $mail->{to}\n$mail->{data}\n";
    },
);

$server->start;
AnyEvent->condvar->recv;
