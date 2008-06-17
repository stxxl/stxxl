#!/usr/bin/perl -w
#
# A tool to analyze #include dependencies and find unwanted cycles.
#
# Copyright (C) 2007 by Andreas Beckmann <beckmann@mpi-inf.mpg.de>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
#

use File::Basename;

$mcstl = 1;
$mcstlpath = 'c++';

$CXX = 'g++-4.2';
$cxxtarget = 'foo-bar-gnu';
$cxxheaderpath = undef;
$gcctargetheaderpath = undef;

@includepath = ();
push @includepath, $mcstlpath if $mcstl;

%breakloops = qw(
	libio.h			_G_config.h
	sys/cdefs.h		features.h
	sys/ucontext.h		signal.h
	debug/formatter.h	debug/debug.h
	debug/bitset		bitset
	debug/deque		deque
	debug/list		list
	debug/map		map
	debug/set		set
	debug/vector		vector
	bits/sstream.tcc	sstream
	debug/hash_map		ext/hash_map
	debug/hash_set		ext/hash_set
	xmmintrin.h		emmintrin.h
	ext/vstring.h		vstring.tcc
	);

%seen = ();
@todo = ();
%in = ();
%out = ();
$out{'MISSING'} = [];

sub get_file_list($;@)
{
	my $path = shift;
	my @patterns = @_;
	@patterns = ('*') unless scalar @patterns;

	my @l;
	foreach my $p (@patterns){
		foreach (glob "$path/$p") {
			if (-f $_) {
				my $x = $path;
				$x =~ s/([+])/\\$1/g;
				s|^$x/||;
				push @l, $_;
			}
		}
	}
	#print "GLOB $path @patterns: @l\n";
	return @l;
}

sub get_cxx_include_paths($)
{
	my $cxx = shift;
	open CXXOUT, "$cxx -E -v -xc++ /dev/null 2>&1 >/dev/null |" or die $!;
	my $ok = 0;
	while (<CXXOUT>) {
		chomp;
		$cxxtarget = $1 if /^Target: (.*)/;
		$ok = 1 if /^#include .\.\.\.. search starts here:$/;
		$ok = 0 if /^End of search list\.$/;
		if ($ok && s/^ //) {
			push @includepath, $_;
			unless ($cxxheaderpath) {
				$cxxheaderpath = $_ if m|/c\+\+|;
			}
			unless ($gcctargetheaderpath) {
				$gcctargetheaderpath = $_ if (m|/$cxxtarget/| && ! m|/c\+\+|);
			}
		}
	}
	close CXXOUT;
	print "TARGET: \t$cxxtarget\n";
	print "HEADERS c++: \t$cxxheaderpath\n";
	print "HEADERS gcc: \t$gcctargetheaderpath\n";
	print "INCLUDES: \t@includepath\n";
}

sub find_header($;$)
{
	my $header = shift;
	my $relpath = dirname(shift || '.');
	foreach $_ (@includepath, $relpath) {
		#print "FOUND: $header as $_/$header\n";
		return [ $header, "$_/$header" ] if -f "$_/$header";
	}
	#print "NOT FOUND: $header\n";
	return [$header, undef];
}

sub parse_header($)
{
	my $arg = shift;
	my $header = $$arg[0];
	my $file = $$arg[1];
	return if $seen{$file || $header};
	$seen{$file || $header} = 1;
	if ($file) {
		print "PROCESSING: $header \t($file)\n";
		print "OUCH: $header\n" if exists $$out{$header};
		$out{$header} = [];
		open HEADER,"<$file" or die $!;
		while ($_ = <HEADER>) {
			if (/^\s*#\s*include\s+["<]([^">]*)[">]/) {
				my $dep_header = $1;
				print "DEP: $header \t$dep_header\n";
				push @{$out{$header}}, $dep_header;
				push @{$in{$dep_header}}, $header;
				push @todo, find_header($dep_header, $file);
			}
		}
		close HEADER;
	} else {
		print "NOT FOUND: $header\n";
		push @{$out{$header}}, 'MISSING';
		push @{$in{'MISSING'}}, $header;
	}
}


get_cxx_include_paths($CXX);

@cxxheaders = get_file_list($cxxheaderpath);
@cxxbwheaders = get_file_list("$cxxheaderpath/backward");
@cxxextheaders = get_file_list($cxxheaderpath, 'ext/*');
@cxxbitsheaders = get_file_list($cxxheaderpath, 'bits/*');
@cxxtargetheaders = get_file_list("$cxxheaderpath/$cxxtarget", '*', 'bits/*');
#@cxxtr1headers = get_file_list($cxxheaderpath, 'tr1/*', 'tr1_impl/*');
@gcctargetheaders = get_file_list($gcctargetheaderpath, '*.h', 'bits/*.h');

@mcstlheaders = get_file_list($mcstlpath);
@mcstlmetaheaders = get_file_list($mcstlpath, 'meta/*');
@mcstlbitsheaders = get_file_list($mcstlpath, 'bits/*');

if ($mcstl) {
push @todo, find_header($_) foreach (
	sort(@mcstlheaders),
	sort(@mcstlmetaheaders),
	sort(@mcstlbitsheaders),
	);
}

push @todo, find_header($_) foreach (
	sort(@cxxheaders),
	sort(@cxxbwheaders),
	sort(@cxxextheaders),
	sort(@cxxbitsheaders),
	sort(@cxxtargetheaders),
	#sort(@cxxtr1headers),
	sort(@gcctargetheaders),
	);

while (@todo) {
	parse_header(shift @todo);
}

%odgr = ();
@zodgr = ();

foreach (sort keys %out) {
	$odgr{$_} = scalar @{$out{$_}};
	push @zodgr, $_ if $odgr{$_} == 0;
}

while (my ($h, $l) = each %breakloops) {
	next unless exists $out{$h};
	foreach (@{$out{$h}}) {
		if ($l eq $_) {
			print "BREAK LOOP: $h --> $l\n";
			--$odgr{$h};
			push @zodgr, $h if $odgr{$h} == 0;
			$_ .= ".UNLOOP";
		}
	}
}

print "TOPSORT:\n";
while (@zodgr) {
	$curr = shift @zodgr;
	next unless exists $out{$curr};
	print "\t$curr\n";
	foreach (@{$in{$curr}}) {
		--$odgr{$_};
		push @zodgr, $_ if $odgr{$_} == 0;
	}
	delete $out{$curr};
}

print "CYCLIC(".(scalar keys %out)."):\n";
foreach (sort keys %out) {
	print "\t$_($odgr{$_}):";
	foreach (@{$out{$_}}) {
		print " $_" if exists $out{$_};
	}
	print "\n";
}

