#!/usr/bin/perl
#
# Perl Chat Client modified to use ssl socket
# Author: Torrance Jones and me fakessh @
# Date: August 23, 2000
# License: GNU General Public License
#    http://www.fsf.org/copyleft/gpl.html
#
#    Copyright (C) 2000 Torrance Jones unless specifically stated
#                       otherwise in the comments of each sub.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
#    USA
#
# Version: 0.1.1 (Alpha)
# Additional Contributors: None Yet! :(
#
# NOTE: if anyone does add functionality to this program please document
#       your changes in each sub as well as place your name and email
#       into that subs comments so you can get proper credit.  if you
#       create a new sub try to follow the general format of the other
#       subs so as to keep the program basically consistent.
#
# Changes:       added ability to change your personal info
#                no longer need to edit $host and $port in the perl
#                   program, all configuration is done through the
#                   the configuration file and Edit->Options.
#                fixed a problem with updating the info from a changed
#                   configuration. 
#          0.1.0 changed procedure calls from create_gui() to accomodate
#                   for the new algorithm and format.
#                rewrite of lower level socket code...
#          0.0.9 Added gif to About popup.
#          0.0.8 Added ability to save user data to .perlchatrc
#                Added support for .perlchatrc config file to hold data
#                   this includes, autoconnect/autologon
#                Added get_user_info, to get buddy info, protocol '8'.
#                fixed license popup (added ok box)
#                added better error popups for registration
#          0.0.7 better handling of messages
#                UI change with messaging
#                squashed some globals in the messaging popups
#                created a new protocol '7' Global Message
#          0.0.6 added "about" pulldown and the license to 
#                  the menu buttons
#                changed the way the output was displayed 
#                  (i.e. to the status window)
#          0.0.5 created gui
#                added Tk module
#          0.0.4 added threads
#
# TODO: better organize the connect_serv() procedure.
#       $conn_stat should be updated sometime regularly
#       Better handling of the variables for the user login
#         and user registration because they are globals and
#         could easily get corrupted.
#       Filter out messages so the client cannot send
#         a '::' to the server in the body of a message to
#         another user.
#       Get rid of globals $pass, $user
#
# Client Side ID Numbers
#  1). logon to server
#  2). register new screenname
#  3). terminate program
#  4). log off server
#  5). get logged users
#  6). message another user
#  7). global message
#  8). query buddy

use Tk;
use IO::Select;
use POSIX;
use IO::Socket::SSL 'inet4';
use Socket;
use Fcntl;
use Tie::RefHash;

my $VERSION = "0.1.1";

my ($user, $pass, $auto, $realname, $email, $quote); 
my $configfile_name = '.perlchatrc';

$| = 1;

retrieve_config();
create_gui();

#---------------------------------------------------------------------
sub create_gui {
# Preconditions: none
# Postconditions: creates the top level window, its frames, the 
#                 buddy list, and the bindings on actions to
#                 objects in this window.

   $top = MainWindow->new();
   $top->title ("Perl Chat $VERSION");

   # Create Menu Bar
   $menu_bar = $top->Frame()->pack('-side' => 'top',
                                   '-fill' => 'x');
 
   # File Menu Button
   $menu_file = $menu_bar->Menubutton('-text'         => 'File',
                                      '-relief'       => 'raised',
                                      '-borderwidth'  => 2,
                                      )->pack('-side' => 'left',
                                              '-padx' => 2
                                              );

   # Edit Menu Button
   $menu_edit = $menu_bar->Menubutton('-text'         => 'Edit',
                                      '-relief'       => 'raised',
                                      '-borderwidth'  => 2,
                                      )->pack('-side' => 'left',
                                              '-padx' => 2
                                              );
   # About Menu Button
   $menu_about = $menu_bar->Menubutton('-text'         => 'About',
                                       '-relief'       => 'raised',
                                       '-borderwidth'  => 2,
                                       )->pack('-side' => 'right',
                                               '-padx' => 2
                                               );

   $menu_about->command('-label'       => 'About',
                       '-accelerator' => ' ',
                       '-underline'   => 0,
                       '-command'     => \&popup_about);

   $menu_about->command('-label'       => 'License',
                       '-accelerator' => ' ',
                       '-underline'   => 0,
                       '-command'     => \&popup_license);

   $menu_edit->command('-label'       => 'Options',
                       '-accelerator' => ' ',
                       '-underline'   => 1,
                       '-command'     => \&options);

   $menu_edit->command('-label'       => 'Clear Messages',
                       '-accelerator' => ' ',
                       '-underline'   => 1,
                       '-command'     => sub { $status->delete('0.0', 'end') });

   $menu_edit->command('-label'       => 'Update Info',
                       '-accelerator' => ' ',
                       '-underline'   => 1,
                       '-command'     => \&update_info);

   $menu_file->command('-label'       => 'Connect',
                       '-accelerator' => ' ',
                       '-underline'   => 0,
                       '-command'     => sub { connect_server($host,$port) });

   $menu_file->command('-label'       => 'Login',
                       '-accelerator' => ' ',
                       '-underline'   => 1,
                       '-command'     => \&login);

   $menu_file->command('-label'       => 'Register',
                       '-accelerator' => '',
                       '-underline'   => 0,
                       '-command'     => \&register);

   $menu_file->command('-label'       => 'List Users',
                       '-accelerator' => '',
                       '-underline'   => 1,
                       '-command'     => \&update_buddy_list);

   $menu_file->command('-label'       => 'Global Message',
                       '-accelerator' => '',
                       '-underline'   => 0,
                       '-command'     => \&msg_all_buddy);

   $menu_file->command('-label'       => 'Log Off',
                       '-accelerator' => '',
                       '-underline'   => 5,
                       '-command'     => \&logoff);

   $menu_file->command('-label'       => 'Get Buddy Info.',
                       '-accelerator' => '',
                       '-underline'   => 1,
                       '-command'     => \&get_user_info);
   
   $menu_file->command('-label'       => 'Quit',
                       '-accelerator' => '',
                       '-underline'   => 0,
                       '-command'     => sub { quit(); exit(1) } );

   $bottom_frame = $top->Frame()->pack(-side => 'bottom',
                                -fill => 'x',
                                -expand => 'x');

   $left_frame = $bottom_frame->Frame()->pack(-side => 'left',
                                              -fill => 'y',
                                              -expand => 'y');

   $right_frame = $bottom_frame->Frame()->pack(-side => 'right',
                                               -fill => 'y',
                                               -expand => 'y');

   # Sample radio buttons/checkboxes/separators
   #$match_type = "regexp"; $case_type = 1;
   #######################################################
   #$menu_file->separator();
   ## Regexp match
   #$menu_file->radiobutton('-label'    => 'Regexp match',
   #                        '-value'    => 'regexp',
   #                        '-variable' => \$match_type);
   ## Exact match
   #$menu_file->radiobutton('-label'    => 'Exact match',
   #                        '-value'    => 'exact',
   #                        '-variable' => \$match_type);
   #######################################################
   #$menu_file->separator();
   ## Ignore case
   #$menu_file->checkbutton('-label'    => 'Ignore case?',
   #                        '-variable' => \$case_type);

#----------------------- Set up status widgit -------------------------------
   $left_frame->Label(-text => "Perl Chat Client ssl socket v$VERSION")
              ->pack(-side => 'top',
                     -anchor => 'nw');
   $status = $left_frame->Text (-width => 40,
                                -height => 30,
                                -wrap => 'word')->pack();

   $status->tagConfigure(section,
      -font => '-adobe-helvetica-bold-r-normal--14-140-75-75-p-82-iso8859-1');

   $status->bind('<Double-1>', \&pick_word);
   $left_frame->Label(-text => 'Global Message:')->pack(-side => 'left');
   $gm_quick = $left_frame->Entry (-width => 26)->pack(-side => 'left');
   $gm_quick->bind('<KeyPress-Return>', sub { 
                                           send_msg_all($gm_quick->get());
                                           $gm_quick->delete(0,'end');
                                        });

# ------------------------ Set up Buddy list --------------------------------
   $right_frame->Label(-text => 'Buddies')
               ->pack(-side => 'top',
                     -anchor => 'ne');
   $buddy_list = $right_frame->Listbox(-width => 10,
                                       -height => 25)->pack();

   $buddy_list->bind('<Double-1>', \&msg_buddy);

   process_config();

   MainLoop();
}
#----------------------------------------------------------------------------
sub process_config {
   print "Processing Config File...\n";
   if ($auto =~ /^y/i) {
      # auto connect, logon, update info
      connect_server($host, $port);
      send_login($user, $pass);
      # send_update();
   }
}
# ---------------------------------------------------------------------------
sub save_options {
   my ($h, $po, $u, $pa, $a, $r, $e, $q) = @_;
      print "Saving configuration...\n";
      open(CONF, ">$configfile_name") || die "Cannot save new conf file $!\n";
      print CONF "# This is the Perl Chat Client config file.\n";
      print CONF "# You may edit this file with a text editor or\n";
      print CONF "# from the edit menu within Perl Chat itself.\n\n";
      print CONF "HOST=$h\n";
      print CONF "PORT=$po\n";
      print CONF "USER=$u\n";
      print CONF "PASS=$pa\n";
      print CONF "AUTO=$a\n\n";
      print CONF "# User Information to be updated upon login\n\n";
      print CONF "REALNAME=$r\n";
      print CONF "EMAIL=$e\n";
      print CONF "QUOTE=$q\n";
      close (CONF) || die "Cannot close saved config file $!\n";
}
# ---------------------------------------------------------------------------
sub options {

   print "Perl Chat Options.\n";
   my ($options_window) = $top->Toplevel;
   $options_window->title ("Perl Chat Options");

   my $bottom_frame = $options_window->Frame()->pack(-side => 'bottom',
                                                    -fill => 'x',
                                                    -expand => 'x');

   my $left_frame = $options_window->Frame()->pack(-side => 'left',
                                                   -fill => 'x',
                                                   -expand => 'x');

   my $right_frame = $options_window->Frame()->pack(-side => 'right',
                                                    -fill => 'x',
                                                    -expand => 'x');
   my($host_label) = $left_frame->Label (
                -anchor => 'w',
                -justify => 'left',
                -text => 'Host',
   )->pack();

   my($host_entry) = $right_frame->Entry (-width => 40)->pack();;

   my($port_label) = $left_frame->Label (
                -anchor => 'w',
                -justify => 'left',
                -text => 'Port',
   )->pack();

   my($port_entry) = $right_frame->Entry (-width => 40)->pack();

   my($user_label) = $left_frame->Label (
                -anchor => 'w',
                -justify => 'left',
                -text => 'Username',
   )->pack();

   my($user_entry) = $right_frame->Entry (-width => 40)->pack();

   my($pass_label) = $left_frame->Label (
                -anchor => 'w',
                -justify => 'left',
                -text => 'Password',
   )->pack();

   my($pass_entry) = $right_frame->Entry (-width => 40,
                                          -show => '*')->pack();

   my($auto_label) = $left_frame->Label (
                -anchor => 'w',
                -justify => 'left',
                -text => 'Auto Login',
   )->pack();

   my($auto_entry) = $right_frame->Entry (-width => 40)->pack();


   my($realname_label) = $left_frame->Label (
                -anchor => 'w',
                -justify => 'left',
                -text => 'Real Name',
   )->pack();

   my($realname_entry) = $right_frame->Entry (-width => 40)->pack();

   my($email_label) = $left_frame->Label (
                -anchor => 'w',
                -justify => 'left',
                -text => 'Email',
   )->pack();

   my($email_entry) = $right_frame->Entry (-width => 40)->pack();

   my($quote_label) = $left_frame->Label (
                -anchor => 'w',
                -justify => 'left',
                -text => 'Quote',
   )->pack();

   my($quote_entry) = $right_frame->Entry (-width => 40)->pack();

   $host_entry->insert (0,"$host");
   $port_entry->insert (0,"$port");
   $user_entry->insert (0,"$user");
   $pass_entry->insert (0,"$pass");
   $auto_entry->insert (0,"$auto");
   $realname_entry->insert (0,"$realname");
   $email_entry->insert (0,"$email");
   $quote_entry->insert (0,"$quote");

   $bottom_frame->Button(-text => 'Save',
                        -command => sub {
                               save_options($host_entry->get(),
                                            $port_entry->get(),
                                            $user_entry->get(),
                                            $pass_entry->get(),
                                            $auto_entry->get(),
                                            $realname_entry->get(),
                                            $email_entry->get(),
                                            $quote_entry->get());
                               $host = $host_entry->get();
                               $port = $port_entry->get();
                               $user = $user_entry->get();
                               $pass = $pass_entry->get();
                               $auto = $auto_entry->get();
                               $realname = $realname_entry->get();
                               $email = $email_entry->get();
                               $quote = $quote_entry->get();

                               destroy $options_window;
                                    }
                           )->pack(-side => 'left');
   $bottom_frame->Button(-text => 'Cancel',
                        -command => sub{destroy $options_window}
                          )->pack(-side => 'right');

}
# ---------------------------------------------------------------------------
sub get_user_info {
# Preconditions: should be connected to a server, and logged into the server
# Postconditions: creates a window, prompts user for a buddy, 
#                 calls send_query with this info (buddy)
#
   my ($get_info_window, $get_buddy);

   print "Getting info to query buddy.\n";
   if(defined $conn) {
      print "Connection Checked: Proceeding with message!\n";
      $get_info_window = $top->Toplevel;
      $get_info_window->title ("Query Buddy");

      $get_info_window->Label(-text => "Enter Buddy:")->pack();

      $get_buddy = $get_info_window->Entry (-width => 20)->pack();

      $get_buddy->bind('<KeyPress-Return>',sub {
                              send_query($get_buddy->get());
                              destroy $get_info_window;
                           });
      $get_info_window->Button(-text => 'OK',
                           -command => sub {
                              send_query($get_buddy->get());
                              destroy $get_info_window;
                           }
                           )->pack(-side => 'left');
      $get_info_window->Button(-text => 'Cancel',
                           -command => sub{destroy $get_info_window}
                             )->pack(-side => 'left');
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
# ---------------------------------------------------------------------------
sub msg_all_buddy {
# Preconditions: should be connected to a server, and logged into the server
# Postconditions: creates a window, prompts user for msg, calls send_msg_all
#                 with this info (message)
#
   my ($msg_all_window, $get_msg);

   print "Getting message to send to $buddy\n";
   if(defined $conn) {
      print "Connection Checked: Proceeding with message!\n";
      $msg_all_window = $top->Toplevel;
      $msg_all_window->title ("Global Message");

      $msg_all_window->Label(-text => "Enter Message:")->pack();

      $get_msg = $msg_all_window->Entry (-width => 40)->pack();

      $get_msg->bind('<KeyPress-Return>',sub { 
                              send_msg_all($get_msg->get());
                              destroy $msg_all_window;
                           });
      $msg_all_window->Button(-text => 'OK',
                           -command => sub { 
                              send_msg_all($get_msg->get()); 
                              destroy $msg_all_window;
                           }
                           )->pack(-side => 'left');
      $msg_all_window->Button(-text => 'Cancel',
                           -command => sub{destroy $msg_all_window}
                             )->pack(-side => 'left');
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
# ---------------------------------------------------------------------------
sub msg_buddy {
# Preconditions: should be connected to server, and logged on to server
# Postconditions: creates a window and gets info from user and calls send_msg
#                 with the info obtained (recipient and message)
#
   my $buddy = $buddy_list->get('active');
   my ($msg_bud_window, $get_msg, $get_rcpt);
   return if(!$buddy);
   print "Getting message to send to $buddy\n";

   if(defined $conn) {
      print "Connection Checked: Proceeding with message!\n";
      $msg_bud_window = $top->Toplevel;
      $msg_bud_window->title ("Message Buddy");

      $msg_bud_window->Label(-text => "To User:")->pack();
      $get_rcpt = $msg_bud_window->Entry (-width => 15)->pack();
      $get_rcpt->insert (0,"$buddy");

      $msg_bud_window->Label(-text => "Enter Message:")->pack();

      $get_msg = $msg_bud_window->Entry (-width => 40)->pack();

      $get_msg->bind('<KeyPress-Return>', sub { 
                                             send_msg($buddy, $get_msg->get());
                                             destroy $msg_bud_window;
                                          });
      $msg_bud_window->Button(-text => 'OK',
                           -command => sub { 
                                          send_msg($buddy, $get_msg->get());
                                          destroy $msg_bud_window;
                                     }
                           )->pack(-side => 'left');
      $msg_bud_window->Button(-text => 'Cancel',
                           -command => sub{destroy $msg_bud_window}
                             )->pack(-side => 'left');
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
   # THis is how to delete a buddy from the list
   # $buddy_list->delete('active'); # delete an actively selected buddy
}
# ---------------------------------------------------------------------------
sub connect_server {
# Preconditions: gui should be created, this should be called from 
#                Connect under the File menu button.
# Postconditions: connects to server then spawns another thread
#                 this new thread processes responses from the server. 
   my ($host, $port) = @_;
#
# connect socket -> set up select -> loop
#
   if($conn_stat ne 'connected') {
      print "Connecting to Server $host:$port...\n";

      $conn = IO::Socket::SSL->new(PeerAddr => $host,
                                 PeerPort => $port,
                                 Proto    => 'tcp',
                                 SSL_use_cert => 1,
                                 SSL_verify_mode => 0x00,
                                 SSL_key_file => '/home/swilting/perltest/private/ks37777.kimsufi.com.key',
                                 SSL_cert_file => '/home/swilting/perltest/certs/ks37777.kimsufi.com.cert',
                                 SSL_ca_file   => '/home/swilting/perltest/certs/ks37777.kimsufi.com.cert',
                                 SSL_passwd_cb => sub { return "" },

)
        or popup_err(4);

###
      %inbuffer  = ();
      %outbuffer = ();
      %ready     = ();
      tie %ready, 'Tie::RefHash';
      nonblock($conn);
      $select = IO::Select->new($conn);   
###
      $menu_file->repeat(5000, sub { wait_for_msgs() });
      $conn_stat = 'connected';
   }

   if (!defined $conn) {
      print "Unhandled error, make a popup.  Could not connect to server.\n";
   }
}
#----------------------------------------------------------------------------
sub login {
# TODO: create frames and make everything line up correctly.
# Preconditions: should be connected to the server
# Postconditions: will log client into server with input provided
   my ($login_window, $get_user, $get_pass);

   if(defined $conn) {
      print "Connection Checked: Proceeding with Login!\n";
      $login_window = $top->Toplevel;
      $login_window->title ("Login Procedure");
      $login_window->Label(-text => 'Enter Your Login Information:')->pack;

      $login_window->Label(-text => "Username:")->pack();
      $get_user = $login_window->Entry (-width => 15)->pack();

      $login_window->Label(-text => "Password:")->pack();
      $get_pass = $login_window->Entry (-width => 15,
                                         -show => '*')->pack();

      $get_pass->bind('<KeyPress-Return>', sub {
                         send_login($get_user->get(), $get_pass->get());
                         destroy $login_window;
                     });

      $login_window->Button(-text => 'OK',
                         -command => sub { 
                            send_login($get_user->get(), $get_pass->get()); 
                            destroy $login_window;
                         }

                           )->pack(-side => 'left');
      $login_window->Button(-text => 'Cancel',
                         -command => sub{destroy $login_window}
                           )->pack(-side => 'left');
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
# ---------------------------------------------------------------------------
sub register {
# Preconditions: none
# Postconditions: draws a window and prompts the user for input.
#                 also calls calls the sub send_reg upon users action

   my $reg_window;

   if(defined $conn) {
      print "Connection Checked: Proceeding with Reg.!\n";
      $reg_window = $top->Toplevel;
      $reg_window->title ("Registration Procedure");
      $reg_window->Label(-text => 'Enter Your Desired Information:')->pack;

      $reg_window->Label(-text => "Username:")->pack();
      $get_new_user = $reg_window->Entry (-width => 15)->pack();

      $reg_window->Label(-text => "Password:")->pack();
      $get_new_pass1 = $reg_window->Entry (-width => 15,
                                         -show => '*')->pack();

      $reg_window->Label(-text => "Re-Enter Password:")->pack();
      $get_new_pass2 = $reg_window->Entry (-width => 15,
                                         -show => '*')->pack();

      $get_new_pass1->bind('<KeyPress-Return>', sub { 
                                     send_reg($get_new_user->get(),
                                              $get_new_pass1->get(), 
                                              $get_new_pass2->get());
                                     destroy $reg_window;
                                  });

      $get_new_pass2->bind('<KeyPress-Return>', sub {
                                     send_reg($get_new_user->get(),
                                              $get_new_pass1->get(),
                                              $get_new_pass2->get());
                                     destroy $reg_window;
                                  });

      $reg_window->Button(-text => 'OK',
                         -command => sub { 
                             send_reg($get_new_user->get(), 
                                      $get_new_pass1->get(),
                                      $get_new_pass2->get()); 
                             destroy $reg_window }
                         )->pack(-side => 'left');

      $reg_window->Button(-text => 'Cancel',
                         -command => sub{destroy $reg_window}
                           )->pack(-side => 'left');
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
#----------------------------------------------------------------------------
sub send_msg_all {
# Preconditions: should have gotten input from user in msg_all_buddy(); before
#                this sub is called.
# Postconditions: sends a message to the server to send a message to
#                 every user

    my ($msg) = @_;

   if(defined $conn) {
      # prints it to console
      print "Global Message:\n";
      print "Contents: $msg\n";

      # Send a the Message to server
      print $conn "7\:\:$user\:\:$pass\:\:$msg\n";
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
#----------------------------------------------------------------------------
sub send_msg {
# Preconditions: should have gotten input from user in msg_buddy(); before
#                this sub is called.
# Postconditions: sends a message to the server to send a message to 
#                 another user

    my ($rcpt, $msg) = @_;

   if(defined $conn) {
      # prints it to console
      print "Recipient: $rcpt\n";
      print "Message: $msg\n";

      # Print message to localhost
      $status->insert('end',"[$user]->[$rcpt]: $msg\n");

      # Send a Private Message
      print $conn "6\:\:$user\:\:$pass\:\:$rcpt\:\:$msg\n";

   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
# ---------------------------------------------------------------------------
sub send_login {
# Preconditions: should have gotten input from user in login(); before
#                this sub is called.
# Postconditions: sends a message to the server to log in
   my ($u, $p) = @_;

   if(defined $conn) {
      if(length($u) > 0 && length($p) > 0) {
         # prints it to console
         print "Username: $u\n";
         print "Password: ********\n";

         print $conn "1\:\:$u\:\:$p\n";
         $user = $u;  # damn these globals, i dont like them!!!!
         $pass = $p;
         update_info();
      } else {
         popup_err(3); 
      }
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
#--------------------------------------------------------------------------
sub send_query {
   my ($buddy) = @_;
  
   if(length($buddy) < 1) {
      popup_err(81);
   } elsif (defined $conn) {
         if (defined $user) {
            # send query to server
            print $conn "8\:\:$user\:\:$pass\:\:$buddy\n";
         } else {
            popup_err(82);
         }
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
#--------------------------------------------------------------------------
sub send_reg {
# Preconditions: should have gotten input from user from the ENTRY box's
#                in register();
# Postconditions: sends a message to the server to register the user
   my ($u, $p1, $p2) = @_;

   if (length($u) < 1) {
      popup_err(25);
   } elsif (length($p1) < 1 || length($p2) < 1) {
      popup_err(26);
   } else {
      if ($p1 eq $p2) {
         if(defined $conn) {
            print "Username: $u\n";
            print "Password: ********\n";

            print $conn "2\:\:$u\:\:$p1\n";
         } else {
            print "No connection established!\n";
            popup_err(91);
         }
      } else {
         popup_err(24);
      }
   }
}
#--------------------------------------------------------------------------
sub logoff {
# Preconditions: should be logged onto server
# Postconditions: sends a message to the server to log user out

   if(defined $conn) {
      print "Logging out of Server\n";
      print "Username: $user\n";
      print "Password: ********\n";
      print $conn "4\:\:$user\:\:$pass\n";
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
#--------------------------------------------------------------------------
sub update_buddy_list {
# Preconditions: none
# Postconditions: sends a message to update the buddy list

   if(defined $conn) {
       print $conn "5\:\:$user\:\:$pass\n";
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
#--------------------------------------------------------------------------
sub process_query {
   my ($msg) = @_;
   my @chopped_msg = split(/::/, $msg);
   my $query_window;
   my $query_msg;

   $query_window = $top->Toplevel;
   $query_window->title ("Query Results");
   $query_window->Label(-text => "User $chopped_msg[3]'s Info Listed Below")->pack;

   $query_msg = $query_window->Text (-width => 60,
                                    -height => 12,
                                      -wrap => 'word')->pack();

   $query_msg->insert('end',"Real Name: $chopped_msg[4]\n");
   $query_msg->insert('end',"Email:     $chopped_msg[5]\n");
   $query_msg->insert('end',"Quote:     $chopped_msg[6]\n");
   $query_msg->insert('end',"IP:        $chopped_msg[7]\n");
   $query_msg->insert('end',"Logged In: $chopped_msg[8]\n");
   $query_msg->insert('end',"Status:    $chopped_msg[9]\n");
   $query_msg->insert('end',"Connected: $chopped_msg[10]\n");

   $query_window->Button(-text => 'OK',
                      -command => sub { destroy $query_window }
                        )->pack();
}
#--------------------------------------------------------------------------
sub wait_for_msgs {
# Preconditions: Must be connected to the Server.
# Postconditions: Process incoming messages from the Server.
   my ($list_size, $msg);
#   my $exit_cond = 1;

#   %inbuffer  = ();
#   %outbuffer = ();
#   %ready     = ();

#   tie %ready, 'Tie::RefHash';

#   nonblock($conn);
#   $select = IO::Select->new($conn);

       my $server;
       my $rv;
       my $data;

       # check for new information on the connections we have
       # anything to read or accept?

       foreach $server ($select->can_read(1)) {
          # read data
          $data = '';
          $menu_file->update;
          $rv   = $server->sysread($data, POSIX::BUFSIZ, 0);

          unless (defined($rv) && length $data) {
              # This would be the end of file, so close the client
              delete $inbuffer{$server};
              delete $outbuffer{$server};
              delete $ready{$server};

              $select->remove($server);
              close $server;
              next;
          }

          $inbuffer{$server} .= $data;

       # test whether the data in the buffer or the data we
       # just read means there is a complete request waiting
       # to be fulfilled.  If there is, set $ready{$client}
       # to the requests waiting to be fulfilled.
          while ($inbuffer{$server} =~ s/(.*\n)//) {
             $menu_file->update;
             push( @{$ready{$server}}, $1 );
          }
       }

       # Any complete requests to process?
       foreach $server (keys %ready) {
          $menu_file->update;
          handle($server);
       }
# This is commented out since we dont care about sending, only receiving!!!
#       # buffers to flush, AKA write to socket!
       foreach $server ($select->can_write(1)) {
          # Skip this client if we have nothing to say
          next unless exists $outbuffer{$server};

          $rv = $server->syswrite($outbuffer{$server}, 0);
          unless (defined $rv) {
#             # Whine, but move on.
             warn "I was told I could write, but I can't.\n";
             next;
          }

          if ($rv == length $outbuffer{$server} ||
             $! == POSIX::EWOULDBLOCK) {
#                  # is executing htis block every time! :(
             substr($outbuffer{$server}, 0, $rv) = '';
             delete $outbuffer{$server} unless length $outbuffer{$server};
          } else {
             # Couldn't write all the data, and it wasn't because
             # it would have blocked.  Shutdown and move on.
             delete $inbuffer{$server};
             delete $outbuffer{$server};
             delete $ready{$server};

            $select->remove($server);
            close($server);
            next;
          }
       }

#       # Out of band data?
       foreach $server ($select->has_exception(0)) {  # arg is timeout
#          # Deal with out-of-band data here, if you want to.
          print "ERROR DEBUG ME!\n";
       }
$top->update;
}
#---------------------------------------------------------------------------
# handle($socket) deals with all pending requests for $client
sub handle {
    # requests are in $ready{$server}
    # send output to $outbuffer{$server}
    my $server = shift;
    my $request;

    foreach $request (@{$ready{$server}}) {
        # $request is the text of the request
        # put text of reply into $outbuffer{$client}
        chomp $request;
        print "Incoming message received: $request\n";
        process_incoming($server, $request);
    }
    delete $ready{$server};
}
# ------------------------------------------------------------------------
# nonblock($socket) puts socket into nonblocking mode
sub nonblock {
    my $socket = shift;
    my $flags;


    $flags = fcntl($socket, F_GETFL, 0)
            or die "Can't get flags for socket: $!\n";
    fcntl($socket, F_SETFL, $flags | O_NONBLOCK)
            or die "Can't make socket nonblocking: $!\n";
}
# ------------------------------------------------------------------------
sub process_incoming {
   my ($server, $msg) = @_;
   my @logged_users;

   my @rcvd_msg = split(/::/, $msg);

   $menu_file->update;

   if ($rcvd_msg[1] eq "1") {
     # Login responses
     # 12 = already logged on
     # 03 = logged in

     if($rcvd_msg[2] eq "03") {
        print "Successfully Logged in!\n";
     } elsif ($rcvd_msg[2] eq "12") {
        popup_err(2);
     } else {
        # Create pop-up for error!
        print "Error Logging in ", $msg, "\n";
        popup_err(1);
     }
     $menu_file->update;
   } elsif ($rcvd_msg[1] eq "2") {
      # register response
      if ($rcvd_msg[2] eq "06") {
         print "New user successfully registered!\n";
         popup_err(27);
      } elsif ($rcvd_msg[2] eq "02") {
         print "$msg\n";
         popup_err(22);
      } else {
         print "$msg\n";
         popup_err(21);
      }      
      $menu_file->update;         
   } elsif ($rcvd_msg[1] eq "3") {
      # quit response
      print "$msg\n";
      $exit_cond = 0;
      $menu_file->update;
   } elsif ($rcvd_msg[1] eq "4") {
      # log out response
      # 14 = user logged off
      # 13 = user not logged in to begin with
      print "$msg\n";
      if($rcvd_msg[2] == 13) {
         popup_err(41);  # not logged in 
      } else {
         # clear the buddy list
         $list_size = $buddy_list->size;
         $list_size = $list_size - 1;
         $buddy_list->delete(0,$list_size);
      }
      $menu_file->update;
   } elsif ($rcvd_msg[1] eq "5") {
      # delete existing list of users 
      $list_size = $buddy_list->size;
      if($list_size > 0) { $buddy_list->delete(0,$list_size); }
         # get users list response
         # if server response for proto 5 is 17 then Draw in $buddy_list
         if ($rcvd_msg[2] == 17) {
            @logged_users = split (/ /, $rcvd_msg[3]);
            foreach (@logged_users) {
               $buddy_list->insert('end', "$_");
            }
         } elsif ($rcvd_msg[2] eq 18) {
            # generate error for login 
            print "Please Log in to server first!\n";
            print "$msg\n";
            popup_err(51);
         } else {
            print "Unknown error updating buddy list:\n";
            print "$msg\n";
            popup_err(52);
         }
         $menu_file->update;
   } elsif ($rcvd_msg[1] eq "6") {
      # receive user message
      # 13 - user not logged in
      # 23 - buddy (target) not logged in
      print "$msg\n";
      rcv_msg($rcvd_msg[3], $rcvd_msg[4]);
   } elsif ($rcvd_msg[1] eq "7") {
      # receive global message
      print "$msg\n";
      rcv_msg_all($rcvd_msg[3], $rcvd_msg[4]);
   } elsif ($rcvd_msg[1] eq "8") {
      if ($rcvd_msg[2] == 23) {
         popup_err(81);
      } elsif ($rcvd_msg[2] eq "13") {
         popup_err(82);
      } else {
         # receive query information
         print "$msg\n";
         process_query($msg);
      }
      $menu_file->update;
   } else {
      print "Unrecognized response: $msg\n";
      popup_err(92);
      exit(0);
   }
   if($err) { print "ERROR: $err\n"; }
}

#------------------------------------------------------------------
sub quit {
# Preconditions: none
# Postconditions: this thread exits

   if(defined $conn) {
      # send quit command
      print $conn "3\:\:$user\:\:$pass\n";
   } else {
      exit;
   }
}
#------------------------------------------------------------------
sub rcv_msg {
   my ($from, $msg) = @_;

   print "Received message from $from\n";     
   if(defined $conn) {
      print "Already Connected: Proceeding with message!\n";
      $status->insert('end',"[$from]: $msg\n");
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
#------------------------------------------------------------------
sub rcv_msg_all {
   my ($from, $msg) = @_;

   print "Received Global message:\n";
   if(defined $conn) {
      print "Already Connected: Proceeding with message!\n";
      $status->insert('end',"$from: $msg\n");
   } else {
      print "No connection established!\n";
      popup_err(91);
   }
}
#------------------------------------------------------------------
sub popup_about {

   my $about_text = "Perl Chat version ssl socket $VERSION\n".
                 "Copyright (C) 2000 Torrance Jones and me fakessh @\n".
                 "Distributed under the GNU GPL\n".
                 "For more information about this program ".
                 "its developers or its license visit\n".
                 "http://perlchat.sourceforge.net or email\n".
                 "torrancejones\@users.sourceforge.net\n";
		  "fakessh\@fakessh.eu\n";

   my $about_window = $top->Toplevel;
   $about_window->title ("About");

   my $topframe = $about_window->Frame(-background => 'White')->pack('-side' => 'top',
                                                                     '-fill' => 'x');
   my $bottomframe = $about_window->Frame(-background => 'White')->pack('-side' => 'bottom',
                                                                        '-fill' => 'x');

   $topframe->Photo('logo',
                -file => "perlchat.gif");

   $topframe->Label('-image' => 'logo')->pack(-side => 'left');

   my $about_msg = $bottomframe->Text (-width => 40,
                                        -height => 10,
                                        -wrap => 'word')->pack();

   $about_msg->insert('end',"$about_text");

   $bottomframe->Button(-text => 'OK',
                      -command => sub { destroy $about_window }
                        )->pack();
}
#------------------------------------------------------------------
sub popup_license {
   my $license_window;
   my $license_msg;

   $license_window = MainWindow->new();
   $license_window->title ("License");
   $license_window->Label(-text => "Perl Chat License")->pack;

   my $top_frame = $license_window->Frame()->pack('-side' => 'top',
                                                  '-fill' => 'x',
                                                   -expand => 'x');

   my $bottom_frame = $license_window->Frame()->pack(-side => 'bottom',
                                                     -fill => 'x',
                                                     -expand => 'x');


   $license_msg = $top_frame->Text (-width => 50,
                                    -height => 15,
                                    -wrap => 'word')->pack(side => 'left',
                                                           padx => 10);

   $license_msg->insert('end',"This program is distributed under the terms ".
     "of the GNU General Public License.  This program is distributed in the ".
     "hope that it will be useful but WITHOUT ANY WARRANTY without even the ".
     "implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR ".
     "PURPOSE.  See the  GNU General Public License for more details.\n\n".
     "You should have received a copy of the GNU General Public License along ".
     "with this program; if not, write to the Free Software Foundation, ".
     "Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA or visit ".
     "http://www.fsf.org/copyleft/gpl.html\nFor more information about this ".
     "program its developers or its license visit\n".
     "http://perlchat.sourceforge.net\nor email ".
     "torrancejones\@users.sourceforge.net\n");

   $scroll = $top_frame->Scrollbar(orient => 'vertical',
                                    width => 10,
                                 command => ['yview', $license_msg]
                                   )->pack(side => 'left',
                                           fill => 'y',
                                           padx => 0);

   $license_msg->configure(yscrollcommand => ['set', $scroll]);

   $bottom_frame->Button(-text => 'OK',
                      -command => sub { destroy $license_window }
                        )->pack();
}
#------------------------------------------------------------------
sub popup_err {
# Preconditions: none
# Postconditions: a popup is drawn according to the error code 
#                 received

   my ($error_code) = @_;
   my $popup_win;
   my $error_msg;
   my $error_title;

   print"Error Code: $error_code\n"; 

   if($error_code eq 1) {
      $error_msg = "Not Connected!";
      $error_title = "Login Error:";
   } elsif($error_code eq 2) {
      $error_msg = "Already Logged on with This User Name!";
      $error_title = "Logon Error:";
   } elsif($error_code eq 3) {
      $error_msg = "Username or Password to short!";
      $error_title = "Logon Error:";
   } elsif($error_code eq 4) {
      $error_msg = "Cannot Create Socket!";
      $error_title = "Socket Error:";
   } elsif($error_code eq 21) {
      $error_msg = "Error Registering User!";
      $error_title = "Registration Error:";
   } elsif($error_code eq 22) {
      $error_msg = "Error user Already Exists!";
      $error_title = "Registration Error:";
   } elsif($error_code eq 24) {
      $error_msg = "Passwords entered do not match!";
      $error_title = "Registration Error:";
   } elsif($error_code eq 25) {
      $error_msg = "Please Enter a Username!";
      $error_title = "Registration Error:";
   } elsif($error_code eq 26) {
      $error_msg = "Please Enter a valid Password!";
      $error_title = "Registration Error:";
   } elsif($error_code eq 27) {
      $error_msg = "User Successfully Added!";
      $error_title = "Registration Message:";
   } elsif($error_code eq 41) {
      $error_msg = "Already Logged Out!";
      $error_title = "Logout Error:";
   } elsif($error_code eq 51) {
      $error_msg = "Please Log into the Server First!";
      $error_title = "Update Error:";
   } elsif($error_code eq 52) {
      $error_msg = "Unkown Update Error!";
      $error_title = "Unknown Update Error:";
   } elsif($error_code eq 81) {
      $error_msg = "Target Not Logged In!";
      $error_title = "Query Error:";
   } elsif($error_code eq 82) {
      $error_msg = "Please Log In First!";
      $error_title = "Query Error:";
   } elsif($error_code eq 91) {
      $error_msg = "No Connection Established!";
      $error_title = "Connection Error:";
   } elsif($error_code eq 92) {
      $error_msg = "Unrecognized Response!";
      $error_title = "Unrecognized Server Response:";
   } else {
      $error_msg = "Unknown Error!";
      $error_title = "Unkown Error:";
   }
   $popup_win = $top->Toplevel;
   $popup_win->title ("$error_title");
   $popup_win->Label(-text => "$error_msg")->pack;

   $popup_win->Button(-text => 'OK',
                      -command => sub {destroy $popup_win})->pack();
}
#------------------------------------------------------------------
sub retrieve_config {
# Preconditions: the config file must follow the proper format,
# Postconditions: global variables are set... ick globals yucky!
#
   print "Retreiving Config File...\n";
   if(-e $configfile_name) {
      # config file exists
      if (-r $configfile_name) {
         # config file is readable
         open (CONF, "$configfile_name") || die "Cannot open config file $!\n";
         while (<CONF>) {
            chomp ($_);
            if($_ =~ /^HOST/i) {
               $_ =~ s/HOST=//;
               $host = $_;
            } elsif ($_ =~ /^PORT/i) {
               $_ =~ s/PORT=//;    
               $port = $_;
            } elsif ($_ =~ /^USER/i) {
               $_ =~ s/USER=//;    
               $user = $_;
            } elsif ($_ =~ /^PASS/i) {
               $_ =~ s/PASS=//;    
               $pass = $_;
            } elsif ($_ =~ /^AUTO/i) {
               $_ =~ s/AUTO=//;
               $auto = $_;
            } elsif ($_ =~ /^REALNAME/i) {
               $_ =~ s/REALNAME=//;
               $realname = $_;
            } elsif ($_ =~ /^EMAIL/i) {
               $_ =~ s/EMAIL=//;
               $email = $_;
            } elsif ($_ =~ /^QUOTE/i) {
               $_ =~ s/QUOTE=//;
               $quote = $_;
            }
         }
         close (CONF) || die "Cannot close config file $!\n";
      }
   } else {
      # create default config file
      open(CONF, ">$configfile_name") || die "Cannot create new conf file $!\n";
      print "No config file exists, so lets create one!\n";
      print CONF "# This is the Perl Chat Client config file.\n";
      print CONF "# You may edit this file with a text editor or\n";
      print CONF "# from the edit menu within Perl Chat itself.\n\n";
      print CONF "HOST=localhost\n";
      print CONF "PORT=6666\n";
      print CONF "USER=enter_your_own_username\n";
      print CONF "PASS=enter_your_own_password\n";
      print CONF "AUTO=no\n\n";
      print CONF "# User Information to be updated upon login\n\n";
      print CONF "REALNAME=Torrance Jones\n";
      print CONF "EMAIL=torrancejones\@users.sourceforge.net\n";
      print CONF "QUOTE=Nothing entered yet.\n";
      close (CONF) || die "Cannot close config file $!\n";
       
      # if this new file is readable...
      if (-r $configfile_name) {
         open (CONF, "$configfile_name") || die "Cannot open config file $!\n";
         while (<CONF>) {
            chomp ($_);
            if($_ =~ /^HOST/i) {
               $_ =~ s/HOST=//;
               $host = $_;
            } elsif ($_ =~ /^PORT/i) {
               $_ =~ s/PORT=//;
               $port = $_;
            } elsif ($_ =~ /^USER/i) {
               $_ =~ s/USER=//;
               $user = $_;
            } elsif ($_ =~ /^PASS/i) {
               $_ =~ s/PASS=//;
               $pass = $_;
            } elsif ($_ =~ /^AUTO/i) {
               $_ =~ s/AUTO=//;
               $auto = $_;
            } elsif ($_ =~ /^REALNAME/i) {
               $_ =~ s/REALNAME=//;
               $realname = $_;
            } elsif ($_ =~ /^EMAIL/i) {
               $_ =~ s/EMAIL=//;
               $email = $_;
            } elsif ($_ =~ /^QUOTE/i) {
               $_ =~ s/QUOTE=//;
               $quote = $_;
            }
         }
         close (CONF) || die "Cannot close config file $!\n";
      }
   }
}
#-----------------------------------------------------------------------------
sub update_info {
# Preconditions: should have gottent the correct values of $realname
#                $email and $quote. 
# Postconditions: sends a message to the server to request info update

   print $conn "9\:\:$user\:\:$pass\:\:$realname\:\:$email\:\:$quote\n";   

}
