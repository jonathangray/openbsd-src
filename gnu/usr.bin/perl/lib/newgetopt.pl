# newgetopt.pl -- new options parsing.
# Now just a wrapper around the Getopt::Long module.
# $Id: newgetopt.pl,v 1.15 1995/12/26 14:57:33 jv Exp $

{   package newgetopt;

    # Values for $order. See GNU getopt.c for details.
    $REQUIRE_ORDER = 0;
    $PERMUTE = 1;
    $RETURN_IN_ORDER = 2;

    # Handle POSIX compliancy.
    if ( defined $ENV{"POSIXLY_CORRECT"} ) {
	$autoabbrev = 0;	# no automatic abbrev of options (???)
	$getopt_compat = 0;	# disallow '+' to start options
	$option_start = "(--|-)";
	$order = $REQUIRE_ORDER;
    }
    else {
	$autoabbrev = 1;	# automatic abbrev of options
	$getopt_compat = 1;	# allow '+' to start options
	$option_start = "(--|-|\\+)";
	$order = $PERMUTE;
    }

    # Other configurable settings.
    $debug = 0;			# for debugging
    $ignorecase = 1;		# ignore case when matching options
    $argv_end = "--";		# don't change this!
}

use Getopt::Long;

################ Subroutines ################

sub NGetOpt {

    $Getopt::Long::debug = $newgetopt::debug 
	if defined $newgetopt::debug;
    $Getopt::Long::autoabbrev = $newgetopt::autoabbrev 
	if defined $newgetopt::autoabbrev;
    $Getopt::Long::getopt_compat = $newgetopt::getopt_compat 
	if defined $newgetopt::getopt_compat;
    $Getopt::Long::option_start = $newgetopt::option_start 
	if defined $newgetopt::option_start;
    $Getopt::Long::order = $newgetopt::order 
	if defined $newgetopt::order;
    $Getopt::Long::ignorecase = $newgetopt::ignorecase 
	if defined $newgetopt::ignorecase;

    &GetOptions;
}

################ Package return ################

1;

################ End of newgetopt.pl ################
