package Sys::Hostname;

use Carp;
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(hostname);

=head1 NAME

Sys::Hostname - Try every conceivable way to get hostname

=head1 SYNOPSIS

    use Sys::Hostname;
    $host = hostname;

=head1 DESCRIPTION

Attempts several methods of getting the system hostname and
then caches the result.  It tries C<syscall(SYS_gethostname)>,
C<`hostname`>, C<`uname -n`>, and the file F</com/host>.
If all that fails it C<croak>s.

All nulls, returns, and newlines are removed from the result.

=head1 AUTHOR

David Sundstrom <sunds@asictest.sc.ti.com>

Texas Instruments

=cut

sub hostname {

  # method 1 - we already know it
  return $host if defined $host;

  if ($^O eq 'VMS') {

    # method 2 - no sockets ==> return DECnet node name
    eval {gethostbyname('me')};
    if ($@) { return $host = $ENV{'SYS$NODE'}; }

    # method 3 - has someone else done the job already?  It's common for the
    #    TCP/IP stack to advertise the hostname via a logical name.  (Are
    #    there any other logicals which TCP/IP stacks use for the host name?)
    $host = $ENV{'ARPANET_HOST_NAME'}  || $ENV{'INTERNET_HOST_NAME'} ||
            $ENV{'MULTINET_HOST_NAME'} || $ENV{'UCX$INET_HOST'}      ||
            $ENV{'TCPWARE_DOMAINNAME'} || $ENV{'NEWS_ADDRESS'};
    return $host if $host;

    # method 4 - does hostname happen to work?
    my($rslt) = `hostname`;
    if ($rslt !~ /IVVERB/) { ($host) = $rslt =~ /^(\S+)/; }
    return $host if $host;

    # rats!
    $host = '';
    Carp::croak "Cannot get host name of local machine";  

  }
  else {  # Unix

    # method 2 - syscall is preferred since it avoids tainting problems
    eval {
	{
	    package main;
	    require "syscall.ph";
	}
	$host = "\0" x 65; ## preload scalar
	syscall(&main::SYS_gethostname, $host, 65) == 0;
    }

    # method 3 - trusty old hostname command
    || eval {
	$host = `(hostname) 2>/dev/null`; # bsdish
    }

    # method 4 - sysV uname command (may truncate)
    || eval {
	$host = `uname -n 2>/dev/null`; ## sysVish
    }

    # method 5 - Apollo pre-SR10
    || eval {
	($host,$a,$b,$c,$d)=split(/[:\. ]/,`/com/host`,6);
    }

    # bummer
    || Carp::croak "Cannot get host name of local machine";  

    # remove garbage 
    $host =~ tr/\0\r\n//d;
    $host;
  }
}

1;
