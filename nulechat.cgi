#! /usr/bin/perl -wT
#
###################################################################
# nulechat.cgi - a chat server written in Perl for the web        #
###################################################################
# Abstract: nulechat is a simple chat server written in Perl      #
#         : that can be the core of a frame chat using a web      #
#         : browser or using a simple application.  Unlike most   #
#         : other web chat servers this one is designed to be     #
#         : programmatically "correct" and (hopefully) secure.    #
#                                                                 #
#         : This program performs no authentication on its own.   #
#         : That is up for your web server to do, or for you to.  #
#                                                                 #
# History : 20020212 - fakessh @ - Initial version.                     #
#                                                                 #
# To-do   : Add features?                                         #
###################################################################
# Boring, pointless crap:                                         #
# This is free software that may be distributed under the same    #
# license as Perl itself.  This software comes with no warranty.  #
#                                                                 #
# I suppose that it's Copyright (C) 2012 fakessh                  #
#                                                                 #
# Get the latest version from                                     #
# http://ns.fakessh.eu/cgi-bin/nulechat.cgi/ which is also        #
# where the author can be reached.                                #
###################################################################
use constant VERSION => "20120212";

use strict;
use CGI qw/:standard/;
use CGI::Carp qw/fatalsToBrowser warningsToBrowser/;
use POSIX;
use Fcntl qw/:flock/;
use AnyDBM_File; # Can probably drop in any file db.

# Define a few things we need.

####################
# Internal defines #
####################
# These are the locations of the two databases we use.
use constant USERS => "/home/webmail/www/users";
use constant CHAT  => "/home/webmail/www/chat";

# Define this to get a lock whenever we write.
use constant LOCK  => "/home/webmail/www/lock";
# Someone please correct me if I am incorrect about this...
die "Can't lock in Win32, set LOCK => undef\n"
    if (LOCK && ($^O eq "MSWin32"));

# These are maximum counts for those databases. (C for concurrent)
use constant CUSERS => 35;
use constant CCHAT => 1000;

########################
# Application defaults #
########################
# This is maximum idle time (seconds) for our users.
use constant TIMEOUT => 1800;
# This is the default refresh rate (seconds).
use constant REFRESH => 15;
# Whether to show times by default or not.
use constant SHOWTIME => 'checked';
# How many lines to show in the chat window by default.
use constant SHOWLINE => 20;

# Define allowed markup substitutions, set equal to undef to disable.
use constant MARKUP => {
    b => "font-weight: bold",
    i => "font-style: italic",
    h => "background-color: yellow",
    red => "color: red",
    green => "color: green",
    blue => "color: blue",
    reverse => "color: white; background-color: black",
    spoiler => "color: black; background-color: black"
};

# Lastly you can define a stylesheet here
# div.title is for the text headers of most sections.
use constant CSS => qq/
    body { background-color: silver }
    div.away { color: blue; font-style: italic; padding: 2px }
    div.items { padding: 2px }
    div.para { padding: 5px }
    div.title { color: white; background-color: blue; font-style: italic; font-size: 150% }
    div.warning { color: white; background-color: blue; border: 10px solid white; font-weight: bold; font-size: 110% }
    span.bold { font-weight: bold }
    span.italic { font-style: italic }
    span.mini { font-style: italic;font-size: 75% }
    table { width: 98% }
    td.content { background-color: white; border: 2px solid gray  }
/;

# I feel so 'matt's script archive' here, but...
#
###################################################################
# Edit not what thou seeths unlesseth thou knowest what one doest #
################(err... something to that affect)##################
# Don't change any of this other shiznit below here in other words.

# This is to make sure we don't o'er reach our DBM related limits.
use constant LENGTH => 950;

###########
# Objects #
###########

my $cgi = new CGI;

my $state = {};

$state->{host} = $cgi->remote_host();

&match(\$cgi, \$state, 'state', 'OTHER');

&match(\$cgi, \$state, 'error', '');

if ($cgi->param('user'))
{
    $state->{user} = substr $cgi->param('user'), 0, 30;
    $state->{user} =~ tr/A-Za-z0-9 _-{}/|/cs;
    $state->{user} =~ tr/ /_/;
    $state->{user} =~ s/\|//g;

    # Examine and update the user database.
    my %users;

#    if (LOCK)
#    {
#        open FILE, ">".LOCK or die "Couldn't open the LOCK file: $!";
#        flock FILE, LOCK_EX;
#    }
#    tie %users, 'AnyDBM_File', USERS, O_RDWR|O_CREAT, 0666
#        or die "Could not tie database: $!";
    my$db = tie(%users, 'DB_File', USERS, O_CREAT|O_RDWR, 0644)
              || die "dbcreat not tie database $!";
    my$fd = $db->fd;
    open(FILE, "+<&=$fd") || die "dup $!";
    flock (FILE, LOCK_EX) || die "flock: $!";
# it is the new feature for lock and unlock the database

    # Store the first open slot available
    my $unused = -1;
    my $matched = -1;

    for my $i (0..CUSERS)
    {
        if (!defined($users{"last$i"}) or !defined($users{"name$i"}) and ($unused < 0))
        {
            $unused = $i;
            next;
        }

        if ($users{"name$i"} eq $state->{user})
        {
            if (($users{"host$i"} eq $state->{host}) && ($users{"last$i"} >= (time - TIMEOUT)))
            {
                # If it's been a little while since a time-update do it here
                # (this is to minimize disk access...)
                if ($users{"last$i"} < ( time - (TIMEOUT / 10) ) )
                {
                    $users{"last$i"} = time;
                }
                $matched = $i;

                $state->{state} = "frame"
                    if ($state->{state} eq "login");
            }
            elsif (($users{"last$i"} < (time - TIMEOUT)))
            {
                if ($unused < 0)
                {
                    $unused = $i;
                }
            }
            else
            {
                $state->{state} = "OTHER";
                $state->{error} = "Name already in use. "
                    .$state->{user}.", "
                    .$users{"host$i"}.", "
                    .$users{"last$i"}.", "
                    .(time - TIMEOUT);
            }

            last;
        }

        if (($users{"last$i"} < (time - TIMEOUT)) and ($unused < 0))
        {
            $unused = $i;
        }
    }

    # User not in database, free slot available and requested a login.
    if (($matched < 0) and ($unused >= 0) and ($state->{state} eq "login"))
    {
        $users{"name$unused"} = $state->{user};
        $users{"last$unused"} = time;
        $users{"host$unused"} = $state->{host};

        $state->{state} = "frame";

        $matched = $unused;
    }
    elsif (($matched < 0) and ($unused < 0))
    {
        $state->{state} = "OTHER";
        $state->{error} = "No available spaces!";
    }
    # else the user was matched.

    # See if a person is away or not
    &match(\$cgi, \$state, 'away', 'off');
    if (($state->{away} eq 'on') and !defined($users{"away$matched"}))
    {
        $users{"away$matched"} = 'on';
    }
    elsif(($state->{away} ne 'on') and defined($users{"away$matched"}))
    {
        delete $users{"away$matched"};
    }

#    untie %users;
#    if (LOCK)
#    {
#        flock FILE, LOCK_UN;
#        close FILE;
#    }
    flock(FILE, LOCK_UN);
    undef $db;
    untie %users;
    close(FILE);

    # With a bonifide user we can accept messages
    if (($matched >= 0) and ($cgi->param('message')))
    {
        $state->{message} = substr $cgi->param('message'), 0, LENGTH;
        $state->{message} =~ s/</&lt;/g;
        $state->{message} =~ s/>/&gt;/g;
    }
    else
    {
        $state->{message} = "";
    }

    # We also need to check for a refresh rate
    &match(\$cgi, \$state, 'refresh', REFRESH);
    $state->{refresh} = REFRESH unless $state->{refresh} >= REFRESH;

    # How many lines do we want to show of the chat?
    &match(\$cgi, \$state, 'showline', SHOWLINE);

    # Display a time stamp with chat messages?
    &match(\$cgi, \$state, 'showtime', 'off');

    $cgi->delete('message');
}
else
{
    # No user so force a return to the logon screen.
    $state->{state} = "OTHER" unless $state->{state} eq 'help';
}

################
# Main Routine #
################

# There are five possible states:
#  1) Not yet logged in.
#  2) Requesting a frame.
#  3) Requesting chat content.
#  4) Posting a message.
#  5) Help window.

if ($state->{state} eq 'frame')
{
    &render_frame(\$cgi, \$state);
}
elsif ($state->{state} eq 'content')
{
    &show_content(\$cgi, \$state);
}
elsif ($state->{state} eq 'message')
{
    &show_entry(\$cgi, \$state);
}
elsif ($state->{state} eq 'help')
{
    &show_help(\$cgi);
}
else
{
    &new_user(\$cgi, \$state);
}

exit;

###############
# Subroutines #
###############

# A sub to handle user creation (the default).
sub new_user
{
    my $cgi = shift;
    my $state = shift;

    print
        $$cgi->header,
        $$cgi->start_html(
            -title => '{NULE} Chat',
            -style => CSS
        ),
        $$cgi->start_table,
        $$cgi->start_Tr,
        $$cgi->start_td( { -class => 'content', -valign => 'top' } ),
        $$cgi->div( { -class => 'title' }, 'Create a new user');

    if (defined($$state->{error}) and ($$state->{error} ne ""))
    {
        print $$cgi->h2("Error: ".$$state->{error});
    }

    print
        $$cgi->start_div( { -class => 'items' } ),
        "Please enter your name (alphanumeric please): ",
        $$cgi->br,
        $$cgi->start_form,
        $$cgi->hidden(
            -name => 'state',
            -value => 'login'
        ),
        $$cgi->textfield(
            -name => 'user',
            -size => 30,
            -maxlenght => 30
        ),
        $$cgi->br,
        "Refresh chat every ",
        $$cgi->popup_menu(
            -name => 'refresh',
            -values => [ 15, 30, 60, 120, 300 ],
            -default => 15
        ),
        " seconds.",
        $$cgi->br,
        "Display last ",
        $$cgi->popup_menu(
            -name => 'showline',
            -values => [ 10, 20, 50, 100, 200 ],
            -default => SHOWLINE
        ),
        " messages.",
        $$cgi->br,
        $$cgi->checkbox(
            -name => 'showtime',
            -checked => SHOWTIME,
            -label => ''
        ),
        "Show time stamp on chat?",
        $$cgi->br,
        $$cgi->submit,
        $$cgi->br,
        $$cgi->a( { -href => $$cgi->url(-relative => 1)."?state=help",
            -target => '_new' },
            "Click here if you need help."
        ),
        $$cgi->end_div,
        $$cgi->start_div( { -class => 'warning' } ),
        "Because of the nature of web chat pages, the owner ",
        "of this website has no control over the content ",
        "contained within.  By entering the chat you acknowledge ",
        "this and agree not to hold the web site owner or the ",
        "ISP responsible for the content contained within.",
        $$cgi->end_div,
        $$cgi->end_form,
        $$cgi->end_td,
        $$cgi->end_Tr,
        $$cgi->end_table;

    print
        $$cgi->br,
        $$cgi->a( { -href => "http://ns.fakessh.eu/cgi-bin/nulechat.cgi/", -target => '_top' },
            "{nulechat.cgi}"
        ),
        " frame chat, v.",
        VERSION,
        $$cgi->end_html;

    print
        $$cgi->end_html;
}

# Render the blank frameset.
sub render_frame
{
    my $cgi = shift;
    my $state = shift;

    print
        $$cgi->header( { -title => "{nulechat.cgi} Chat" } ),
        "<frameset rows=\"88%,12%\">",
        "<noframes>Sorry, you need frames.</noframes>",
        "<frame src=\"",
        $$cgi->url(-relative => 1),
        "?state=content&amp;user=$$state->{user}&amp;",
        "refresh=$$state->{refresh}&amp;showline=$$state->{showline}&amp;",
        "showtime=$$state->{showtime}\" />",
        "<frame src=\"",
        $$cgi->url(-relative => 1),
        "?state=message&amp;user=$$state->{user}\" />",
        "</frameset>";
}

# Show the chat content.
sub show_content
{
    my $cgi = shift;
    my $state = shift;

    print
        $$cgi->header,
        $$cgi->start_html(
            -title => '{nulechat.cgi} Chat',
            -style => CSS
        ),
        $$cgi->meta(
            { -http_equiv => "refresh", -content => 
            "$$state->{refresh};url=" .
            $$cgi->url(-relative => 1) . 
            "?state=content&user=$$state->{user}&refresh=$$state->{refresh}&" .
            "showline=$$state->{showline}&showtime=$$state->{showtime}&" .
            "away=$$state->{away}",
            -target => "_self" }
        ),
        $$cgi->start_table,
        $$cgi->start_form( { -target => '_self' } ),
        $$cgi->start_Tr,
        $$cgi->start_td( { -class => 'content', -valign => 'top' } );

    # Chat messages go here
    print
        $$cgi->div( { -class => 'title' },
            "Messages at " .
            strftime('%Y/%m/%d %H:%M:%S', localtime) .
            ": "
        );

    &show_messages($cgi, $state);

    print
        $$cgi->end_td,
        $$cgi->start_td( { -class => 'content', -valign => 'top' } );

    # User list goes here
    print
        $$cgi->div( { -class => 'title' }, "Users: " );

    &show_users($cgi);

    print
        $$cgi->end_td,
        $$cgi->end_Tr,
        $$cgi->start_Tr,
        $$cgi->start_td( { -class => 'content', -colspan => 2 } );

    # Options window goes here
    print
        $$cgi->div( { -class => 'title' },
            "Controls: "
        ),
        $$cgi->hidden(
            -name => 'user',
            -value => $$state->{user}
        ),
        $$cgi->hidden(
            -name => 'state',
            -value => 'content'
        ),
        "Refresh chat every ",
        $$cgi->popup_menu(
            -name => 'refresh',
            -values => [ 15, 30, 60, 120, 300 ],
            -default => 15
        ),
        " seconds.",
        $$cgi->br,
        "Display last ",
        $$cgi->popup_menu(
            -name => 'showline',
            -values => [ 10, 20, 50, 100, 200 ],
            -default => SHOWLINE
        ),
        " messages.",
        $$cgi->br,
        $$cgi->checkbox(
            -name => 'showtime',
            -label => ''
        ),
        "Show time stamp on chat?",
        $$cgi->br,
        $$cgi->checkbox(
            -name => 'away',
            -label => ''
        ),
        "Select here to be marked as away.",
        $$cgi->br,
        $$cgi->submit,
        $$cgi->end_form;

    print
        $$cgi->end_td,
        $$cgi->end_Tr,
        $$cgi->end_table,
        $$cgi->end_html;
}

# Show users.
sub show_users
{
    my $cgi = shift;

    my %users;

    tie %users, 'AnyDBM_File', USERS, O_RDONLY|O_CREAT, 0666
        or die "Could not tie database: $!";

    for (my $i = 0; $i <= CUSERS; $i++)
    {
        # This array makes the background progressively the longer
        # it has been since an update has been received from a person.
        my %colors = (
            9 => "#FFFFFF",
            8 => "#FFFFFF",
            7 => "#EEEEEE",
            6 => "#DDDDDD",
            5 => "#CCCCCC",
            4 => "#BBBBBB",
            3 => "#AAAAAA",
            2 => "#999999",
            1 => "#888888",
            0 => "#777777",
        );

        if (defined($users{"name$i"}) and defined($users{"last$i"}) and defined($users{"host$i"}))
        {
            my $alt = int( 9 * ( $users{"last$i"} + TIMEOUT - time ) / TIMEOUT );
            next if $alt < 0;
            $alt = 0 if $alt < 0;

            my $class = 'items';
            if (defined($users{"away$i"}) and ($users{"away$i"} eq 'on'))
            {
                $class = 'away';
            }

            print
                $$cgi->start_div( { -class => $class, -style => "background-color: ".$colors{$alt} } );
            print "Away (" if $class eq 'away';

            print
                $users{"name$i"},
                " \@ ",
                $users{"host$i"};

            print ")" if $class eq 'away';

            print $$cgi->end_div;

        }
    }


    untie %users;
}

# Show messages.
sub show_messages
{
    my $cgi = shift;
    my $state = shift;

    my %chat;

    tie %chat, 'AnyDBM_File', CHAT, O_RDONLY|O_CREAT, 0666
        or print "Could not tie database: $!<br>There may not be any messages yet.";

    if (defined($chat{current}))
    {
        my $i = $chat{current};
        my $j = $$state->{showline};

        while (($i >= 0) and ($j > 0))
        {
            if (defined($chat{"seq$i"}))
            {
                my @message = split /\|/, $chat{"seq$i"}, 3;

                print
                    $$cgi->start_div( { -class => 'items' } );

                # If the markup feature is enabled, perform the substitutions here
                if (MARKUP)
                {
                    my $hashRef = MARKUP;
                    $message[2] =~ s#\[(\w+):([^\]]+)\]#<span style="$hashRef->{$1}">$2</span>#g;
                }

                # If the message contains a "://" attempt to linkify it.
                $message[2] =~ s#([^\s]+://[^\s]+)#<a href="$1" target="_new">$1</a>#g;
                # If the message starts with "/me" do an emote.
                if ($message[2] =~ s/\s*\/me//)
                {
                    print
                        $$cgi->span( { -style => "font-style: italic" },
                            $message[1], " ", $message[2] );
                }
                else
                {
                    print
                        $$cgi->span( { -style => "font-weight: bold" },
                            $message[1] ),
                        ": ", $message[2];
                }

                if ($$state->{showtime} eq 'on')
                {
                    print
                        $$cgi->br,
                        $$cgi->span( { -class => 'mini' },
                            "Time: ",
                            strftime('%Y/%m/%d %H:%M:%S', localtime($message[0]))
                        );
                }

                print
                    $$cgi->end_div;

                $j -= 1; # This decrements the *max* counter.

                $i -= 1; # The grabs the previous line
                if ($i < 0)
                {
                    $i = CCHAT - 1;
                }
            }
            else
            {
                $i = -1;
                last;
            }
        }
    }

    untie %chat;
}

# Show the entry dialog.
sub show_entry
{
    my $cgi = shift;
    my $state = shift;

    # message is defined, if its not blank, add it.
    if ($$state->{message} ne "")
    {
        my %chat;

        if (LOCK)
        {
            open FILE, ">".LOCK or die "Couldn't open the LOCK file: $!";
            flock FILE, LOCK_EX;
        }
        tie %chat, 'AnyDBM_File', CHAT, O_RDWR|O_CREAT, 0666
            or die "Could not tie database: $!";

        if (!defined($chat{current}))
        {
            $chat{current} = 0;
        }
        else
        {
            $chat{current} += 1;
        }

        if ($chat{current} >= CCHAT)
        {
            $chat{current} = 0;
        }

        $chat{"seq$chat{current}"} = 
            time."|".$$state->{user}."|".$$state->{message};

        untie %chat;
        if (LOCK)
        {
            flock FILE, LOCK_UN;
            close FILE;
        }
    }

    print
        $$cgi->header,
        $$cgi->start_html(
            -title => '{nulechat.cgi} Chat',
            -style => CSS
        ),
        $$cgi->start_form( { -target => '_self' } ),
        $$cgi->hidden(
            -name => 'state',
            -value => 'message'
        ),
        $$cgi->hidden(
            -name => 'user',
            -value => $$state->{user}
        ),
        $$cgi->textfield(
            -name => 'message',
            -default => '',
            -size => 80,
            -maxlength => LENGTH
        ),
        $$cgi->submit,
        $$cgi->br,
        $$cgi->a( { -href => $$cgi->url(-relative => 1)."?state=login",
            -target => '_top' },
            "Log off: ",
            $$state->{user}
        ),
        " | ",
        $$cgi->a( { -href => $$cgi->url(-relative => 1)."?state=help",
            -target => '_new' },
            "Help"
        ),
        " | ",
        $$cgi->a( { -href => "http://ns.fakessh.eu/cgi-bin/nulechat.cgi", -target => '_top' },
            "{nulechat.cgi}"
        ),
        " frame chat, v.",
        VERSION,
        $$cgi->end_html;
}

# Show helpful (erm..) information
sub show_help
{
    my $cgi = shift;

    print
        $$cgi->header,
        $$cgi->start_html(
            -title => '{nulechat.cgi} Chat',
            -style => CSS
        ),
        $$cgi->start_table,
        $$cgi->start_Tr,
        $$cgi->start_td( { -class => 'content', -valign => 'top' } ),
        $$cgi->div( { -class => 'title' }, '{nulechat.cgi} Frame Chat Help');

    print
        $$cgi->start_div( { -class => 'para' } ),
        "Welcome to the {nulechat.cgi} frame chat application. ",
        "This is designed to be an easy to use, easy to install ",
        "application, but it still is relatively secure and I ",
        "made every attempt to write very correct Perl here. In ",
        "otherwords to form a model that you could base your own ",
        "applications off of.",
        $$cgi->end_div;

    print
        $$cgi->start_div( { -class => 'para' } ),
        "First the boring stuff - this application is Copyright (C) ",
        "2002 M. Litherland.  It may be distributed under the same ",
        "terms as Perl itself - so see the ",
        $$cgi->a( { -href => "http://www.perl.com/", -target => '_top' },
            "Perl home page"
        ),
        " for more information.  It comes with no warranty ",
        "what-so-ever.  So feel free to use it, but I can't promise ",
        "that it's safe to use or won't damage your data, etc. ",
        "Never the less, I find it works reasonably well, and you ",
        "most-likely will too.  The author can be reached at ",
        $$cgi->a( { -href => "http://ns.fakessh.eu/cgi-bin/nulechat.cgi", -target => '_top' },
            "{nulechat.cgi}"
        ),
        " which is also where the latest version of the software may ",
        "be found.",
        $$cgi->end_div;

    print
        $$cgi->start_div( { -class => 'para' } ),
        $$cgi->span( { -class => 'bold' }, 'Basic help.' ),
        $$cgi->br,
        "This version of the {nulechat.cgi} frame chat isn't very ",
        "complicated.  Basically from the front screen, pick a ",
        "user name and log on.  There may only be one user with ",
        "a given name, so if it is in use, you will be informed ",
        "and have to pick another.  A name is held for some period ",
        "of time after the user logs off to prevent someone from ",
        "from stealing another's identity. Also important to note ",
        "is that some characters are illegal in names, and if ",
        "try to use them, they will be either removed from your ",
        "name or substituted with something legal.",
        $$cgi->end_div;

    print
        $$cgi->start_div( { -class => 'para' } ),
        $$cgi->span( { -class => 'bold' }, 'Options.' ),
        $$cgi->br,
        "When you log on you have the choice of setting a few ",
        "options. You can set how often you would like your ",
        "messages window reset - around 15 seconds is usually ",
        "quick enough to follow all but the most fast-paced chat ",
        "rooms. If you aren't actively following the chat or it is ",
        "slow-paced, setting it lower will use less band-width. ",
        "You may also select how many lines you wish to display, ",
        "and whether or not you wish to see the time at which ",
        "the message was left. You may also change these settings ",
        "after you have logged in by changing them in the form at ",
        "the bottom of your messages window.  Here you also have ",
        "the option to mark yourself as away, but clicking in that ",
        "checkbox. The only thing that option does is change the ",
        "appearance of your name in the users window.",
        $$cgi->end_div;

    print
        $$cgi->start_div( { -class => 'para' } ),
        $$cgi->span( { -class => 'bold' }, 'Chatting.' ),
        $$cgi->br,
        "Once you have logged on and have your chatting window ",
        "up simply type your messages and go!  Depending upon ",
        "how your web master has configured the application there ",
        "may be a few options available to you to modify the ",
        "appearance of your messages. First is that in grand IRC ",
        "tradition the '/me' prefix allows you to 'emote'. ",
        "If you don't know what that does, try it to see for ",
        "yourself.  The other built-in feature is the auto-linkify ",
        "function which is employed whenever you enter something ",
        "that looks like a URL. No gaurantee that this will turn ",
        "what you type into a proper URL, but it will try. E-mail ",
        "addresses are not linkified, because of the risk of ",
        "posting your e-mail on the web. Do it if you like, but I ",
        "don't want to help spammers.",
        $$cgi->end_div;

    print
        $$cgi->start_div( { -class => 'para' } ),
        $$cgi->span( { -class => 'bold' }, 'Markup.' ),
        $$cgi->br,
        "HTML is stripped from all messages and displayed ",
        "literally. Sorry, but it would do no good to let you ",
        "run arbitrary javascript on another's machine. To ",
        "compensate for that there is an option for the server ",
        "admin to define specialized markups for you to employ. ",
        "The format of these markups is always the same, but ",
        "may be customized by your server administrator. ",
        "The general format is as follows: ",
        $$cgi->br,
        $$cgi->span( { -class => 'bold' }, '[tag:Text I wish to mark.]' ),
        $$cgi->br,
        "which would display 'Text I wish to mark.' in the format ",
        "specified by tag. If tag is not a valid markup code, no ",
        "formatting will be done. If markups are enabled on this ",
        "server a list of valid ones will appear here.",
        $$cgi->end_div;

    if (MARKUP)
    {
        print
            $$cgi->start_table,
            $$cgi->start_Tr,
            $$cgi->th( 'Tag' ),
            $$cgi->th( 'Style code' ),
            $$cgi->th( 'Sample output' ),
            $$cgi->end_Tr;

        foreach (sort keys %${\MARKUP})
        {
            print
                $$cgi->start_Tr,
                $$cgi->start_td( { -class => 'content' } ),
                $$cgi->span( { -class => 'bold' }, "$_" ),
                $$cgi->end_td,
                $$cgi->start_td( { -class => 'content' } ),
                MARKUP->{$_},
                $$cgi->end_td,
                $$cgi->start_td( { -class => 'content' } ),
                $$cgi->span( { -style => MARKUP->{$_} }, 'A sample of the markup' ),
                $$cgi->end_td,
                $$cgi->end_Tr;            
        }

        print
            $$cgi->end_table;
            
    }

    print
        $$cgi->start_div( { -class => 'para' } ),
        $$cgi->span( { -class => 'bold' }, 'Enjoy.' ),
        $$cgi->br,
        "That's most of what there is to know.  Now go and enjoy ",
        "using the {nulechat.cgi} frame chat.",
        $$cgi->end_div;

    print
        $$cgi->start_div( { -class => 'para' } ),
        $$cgi->span( { -class => 'bold' }, 'Webmasters.' ),
        $$cgi->br,
        "Setting up the appliction isn't too difficult.  The only ",
        "thing you must change are the locations of the USERS, CHAT ",
        "and LOCK files. Please be safe and make sure these are in ",
        "a location that is not directly accessable by a web-browser. ",
        "Also in a Win32 environment, you will have to set LOCK equal ",
        "to undef.  The other constants you may modify are notated in ",
        "the source.  I haven't tried every possible combinations of ",
        "variables to modify, but most should be pretty safe to do. Good ",
        "luck and if you get too stuck or notice a serious problem, ",
        "please contact me on my web site listed below.",
        $$cgi->end_div;

    print
        $$cgi->end_td,
        $$cgi->end_Tr,
        $$cgi->end_table;

    print
        $$cgi->br,
        $$cgi->a( { -href => "http://ns.fakessh.eu/cgi-bin/nulechat.cgi", -target => '_top' },
            "{nule.org}"
        ),
        " frame chat, v.",
        VERSION,
        $$cgi->end_html;

    print
        $$cgi->end_html;
}

# Utility function for matching cgi parameters
sub match
{
    my $cgi = shift;
    my $state = shift;
    my ($parameter, $default) = @_;

    if ($$cgi->param("$parameter"))
    {
        $$state->{$parameter} = $$cgi->param("$parameter");
    }
    else
    {
        $$state->{$parameter} = $default;
    }

    return 1;
}
