#!/usr/bin/perl -w
############################################################################
#  misc/analyze-source.pl
#
#  Perl script to test source header files, license headers and write
#  AUTHORS from copyright statements.
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

# print multiple email addresses
my $email_multimap = 0;

# launch emacsen for each error
my $launch_emacs = 0;

# write changes to files (dangerous!)
my $write_changes = 0;

# uncrustify program
my $uncrustify;

# function testing whether to uncrustify a path
sub filter_uncrustify($) {
    my ($path) = @_;

    return 0 if $path =~ m"^include/stxxl/bits/containers/matrix";
    # misformats config.h.in!
    return 0 if $path =~ m"^include/stxxl/bits/config.h.in";
    # doesnt work with comments
    return 0 if $path =~ m"^doc/";

    return 1;
}

use strict;
use warnings;
use Text::Diff;
use File::stat;
use List::Util qw(min);

my %include_list;
my %include_map;
my %authormap;

my @source_filelist;

sub expect_error($$$$) {
    my ($path,$ln,$str,$expect) = @_;

    print("Bad header line $ln in $path\n");
    print("Expected $expect\n");
    print("Got      $str\n");

    system("emacsclient -n $path") if $launch_emacs;
}

sub expect($$\@$) {
    my ($path,$ln,$data,$expect) = @_;

    if ($$data[$ln] ne $expect) {
        expect_error($path,$ln,$$data[$ln],$expect);
        # insert line with expected value
        splice(@$data, $ln, 0, $expect);
    }
}

sub expectr($$\@$$) {
    my ($path,$ln,$data,$expect,$replace_regex) = @_;

    if ($$data[$ln] ne $expect) {
        expect_error($path,$ln,$$data[$ln],$expect);
        # replace line with expected value if like regex
        if ($$data[$ln] =~ m/$replace_regex/) {
            $$data[$ln] = $expect;
        } else {
            splice(@$data, $ln, 0, $expect);
        }
    }
}

sub expect_re($$\@$) {
    my ($path,$ln,$data,$expect) = @_;

    if ($$data[$ln] !~ m/$expect/) {
        expect_error($path, $ln, $$data[$ln], "/$expect/");
    }
}

# check equality of two arrays
sub array_equal {
    my ($a1ref,$a2ref) = @_;

    my @a1 = @{$a1ref};
    my @a2 = @{$a2ref};

    return 0 if scalar(@a1) != scalar(@a2);

    for my $i (0..scalar(@a1)-1) {
        return 0 if $a1[$i] ne $a2[$i];
    }

    return 1;
}

# run $text through a external pipe (@program)
sub filter_program {
    my $text = shift;
    my @program = @_;

    # fork and read output
    my $child1 = open(my $fh, "-|") // die("$0: fork: $!");
    if ($child1 == 0) {
        # fork and print text
        my $child2 = open(STDIN, "-|") // die("$0: fork: $!");
        if ($child2 == 0) {
            print $text;
            exit;
        }
        else {
            exec(@program) or die("$0: exec: $!");
        }
    }
    else {
        my @output = <$fh>;
        close($fh) or warn("$0: close: $!");
        return @output;
    }
}

sub process_cpp {
    my ($path) = @_;

    # check permissions
    my $st = stat($path) or die("Cannot stat() file $path: $!");
    if ($st->mode & 0133) {
        print("Wrong mode ".sprintf("%o", $st->mode)." on $path\n");
        if ($write_changes) {
            chmod(0644, $path) or die("Cannot chmod() file $path: $!");
        }
    }

    # read file
    open(F, $path) or die("Cannot read file $path: $!");
    my @data = <F>;
    close(F);

    unless ($path =~ /\.dox$/) {
        push(@source_filelist, $path);
    }

    my @origdata = @data;

    # put all #include lines into the include_map
    foreach my $ln (@data)
    {
        if ($ln =~ m!\s*#\s*include\s*([<"]\S+[">])!) {
            $include_list{$1} = 1;
            $include_map{$1}{$path} = 1;
        }
    }

    # check #include "stxxl..." use
    {
        foreach my $ln (@data)
        {
            if ($ln =~ m@\s*#\s*include\s*"stxxl\S+"@) {
                print("#include \"stxxl...\" found in $path\n");
                print $ln."\n";
                system("emacsclient -n $path") if $launch_emacs;
            }
        }
    }

    # check for assert() in test cases
    if ($path =~ /^test/)
    {
        foreach my $ln (@data)
        {
            if ($ln =~ m!\bassert\(!) {
                print("found assert() in test $path\n");
            }
        }
    }

    # check for \brief doxygen commands
    {
        foreach my $ln (@data)
        {
            if ($ln =~ m!\\brief!) {
                print("found brief command in $path\n");
                system("emacsclient -n $path") if $launch_emacs;
            }
        }
    }

    # check for @-style doxygen commands
    {
        foreach my $ln (@data)
        {
            if ($ln =~ m!\@(param|tparam|return|result|c|i)\s!) {
                print("found \@-style doxygen command in $path\n");
                system("emacsclient -n $path") if $launch_emacs;
            }
        }
    }

    # check for double underscores
    {
        foreach my $ln (@data)
        {
            next if $ln =~ /^\s*#(if|elif|define|error)/;
            next if $path eq "include/stxxl/bits/common/types.h";

            if ($ln =~ m@\s__(?!(gnu_parallel|gnu_cxx|glibcxx|cxa_demangle|typeof__|attribute__|sync_add_and_fetch|sync_sub_and_fetch|FILE__|LINE__|FUNCTION__))@) {
                print("double-underscore found in $path\n");
                print $ln."\n";
                system("emacsclient -n $path") if $launch_emacs;
            }
        }
    }

    foreach my $ln (@data)
    {
        # replace all NULL with nullptr
        if ($ln =~ /\bNULL\b/ && $ln !~ /NOLINT/) {
            $ln =~ s/\bNULL\b/nullptr/g;
        }
    }

    # check source header
    my $i = 0;
    if ($data[$i] =~ m!// -.*- mode:!) { ++$i; } # skip emacs mode line
    expect($path, $i, @data, "/".('*'x75)."\n"); ++$i;
    expect($path, $i, @data, " *  $path\n"); ++$i;
    expect($path, $i, @data, " *\n"); ++$i;

    # skip over comment
    while ($data[$i] !~ /^ \*  Part of the STXXL/) {
        expect_re($path, $i, @data, '^ \*(  .*)?\n$');
        die unless ++$i < @data;
    }

    # check "Part of STXXL"
    expect($path, $i-1, @data, " *\n");
    expect($path, $i, @data, " *  Part of the STXXL. See http://stxxl.org\n"); ++$i;
    expect($path, $i, @data, " *\n"); ++$i;

    # read authors
    while ($data[$i] =~ /^ \*  Copyright \(C\) ([0-9-]+(, [0-9-]+)*) (?<name>[^0-9<]+)( <(?<mail>[^>]+)>)?\n/) {
        #print "Author: $+{name} - $+{mail}\n";
        $authormap{$+{name}}{$+{mail} || ""} = 1;
        die unless ++$i < @data;
    }

    # otherwise check license
    expect($path, $i, @data, " *\n"); ++$i;
    expect($path, $i, @data, " *  Distributed under the Boost Software License, Version 1.0.\n"); ++$i;
    expect($path, $i, @data, " *  (See accompanying file LICENSE_1_0.txt or copy at\n"); ++$i;
    expect($path, $i, @data, " *  http://www.boost.org/LICENSE_1_0.txt)\n"); ++$i;
    expect($path, $i, @data, " ".('*'x74)."/\n"); ++$i;

    # check include guard name
    if ($path =~ m!^include/stxxl/bits/.*\.(h|h.in)$!)
    {
        expect($path, $i, @data, "\n"); ++$i;

        # construct include guard macro name: STXXL_FILE_NAME_HEADER
        my $guard = $path;
        $guard =~ s!include/stxxl/bits/!stxxl/!;
        $guard =~ tr!/!_!;
        $guard =~ s!\.h(\.in)?$!!;
        $guard = uc($guard)."_HEADER";
        #print $guard."\n";

        expectr($path, $i, @data, "#ifndef $guard\n", qr/^#ifndef /); ++$i;
        expectr($path, $i, @data, "#define $guard\n", qr/^#define /); ++$i;

        my $n = scalar(@data)-1;
        if ($data[$n] =~ m!// vim:!) { --$n; } # skip vim
        expectr($path, $n, @data, "#endif // !$guard\n", qr/^#endif /);
    }

    # run uncrustify if in filter
    if (filter_uncrustify($path))
    {
        my $data = join("", @data);
        @data = filter_program($data, $uncrustify, "-q", "-c", "misc/uncrustify.cfg", "-l", "CPP");

        # manually add blank line after "namespace xyz {" and before "} // namespace xyz"
        my @namespace;
        for(my $i = 0; $i < @data-1; ++$i)
        {
            if ($data[$i] =~ m!^namespace (\S+) \{!) {
                push(@namespace, $1);
                if ($data[$i+1] !~ m!^namespace!) {
                    splice(@data, $i+1, 0, "\n"); ++$i;
                }
            }
            elsif ($data[$i] =~ m!^namespace \{!) {
                push(@namespace, "");
            }
            elsif ($data[$i] =~ m!^}\s+//\s+namespace\s+(\S+)\s*$!) {
                if (@namespace == 0) {
                    print "$path\n";
                    print "    NAMESPACE UNBALANCED! @ $i\n";
                }
                else {
                    # quiets wrong uncrustify indentation
                    $data[$i] =~ s!}\s+//\s+namespace!} // namespace!;
                    expectr($path, $i, @data, "} // namespace ".$namespace[-1]."\n",
                            qr!^}\s+//\s+namespace!);
                }
                if ($data[$i-1] !~ m!^}\s+// namespace!) {
                    splice(@data, $i, 0, "\n"); ++$i;
                }
                pop(@namespace);
            }
            elsif ($data[$i] =~ m!^}\s+//\s+namespace\s*$!) {
                if (@namespace == 0) {
                    print "$path\n";
                    print "    NAMESPACE UNBALANCED! @ $i\n";
                }
                else {
                    # quiets wrong uncrustify indentation
                    $data[$i] =~ s!}\s+//\s+namespace!} // namespace!;
                    expectr($path, $i, @data, "} // namespace\n",
                            qr!^}\s+//\s+namespace!);
                }
                if ($data[$i-1] !~ m!^}\s+// namespace!) {
                    splice(@data, $i, 0, "\n"); ++$i;
                }
                pop(@namespace);
            }
            # no indentation after #endif
            elsif ($data[$i] =~ m!^#endif\s+(//.*)$!) {
                splice(@data, $i, 1, "#endif $1\n");
            }
        }
        if (@namespace != 0) {
            print "$path\n";
            print "    NAMESPACE UNBALANCED!\n";
        }
    }

    return if array_equal(\@data, \@origdata);

    print "$path\n";
    print diff(\@origdata, \@data);

    if ($write_changes)
    {
        open(F, "> $path") or die("Cannot write $path: $!");
        print(F join("", @data));
        close(F);
    }
}

sub process_pl_cmake {
    my ($path) = @_;

    # check permissions
    if ($path !~ /\.pl$/) {
        my $st = stat($path) or die("Cannot stat() file $path: $!");
        if ($st->mode & 0133) {
            print("Wrong mode ".sprintf("%o", $st->mode)." on $path\n");
            if ($write_changes) {
                chmod(0644, $path) or die("Cannot chmod() file $path: $!");
            }
        }
    }

    # read file
    open(F, $path) or die("Cannot read file $path: $!");
    my @data = <F>;
    close(F);

    # check source header
    my $i = 0;
    if ($data[$i] =~ m/#!/) { ++$i; } # bash line
    expect($path, $i, @data, ('#'x76)."\n"); ++$i;
    expect($path, $i, @data, "#  $path\n"); ++$i;
    expect($path, $i, @data, "#\n"); ++$i;

    # skip over comment
    while ($data[$i] !~ /^#  Part of the STXXL/) {
        expect_re($path, $i, @data, '^#(  .*)?\n$');
        die unless ++$i < @data;
    }

    # check "Part of STXXL"
    expect($path, $i-1, @data, "#\n");
    expect($path, $i, @data, "#  Part of the STXXL. See http://stxxl.org\n"); ++$i;
    expect($path, $i, @data, "#\n"); ++$i;

    # read authors
    while ($data[$i] =~ /^#  Copyright \(C\) ([0-9-]+(, [0-9-]+)*) (?<name>[^0-9<]+)( <(?<mail>[^>]+)>)?\n/) {
        #print "Author: $+{name} - $+{mail}\n";
        $authormap{$+{name}}{$+{mail} || ""} = 1;
        die unless ++$i < @data;
    }

    # otherwise check license
    expect($path, $i, @data, "#\n"); ++$i;
    expect($path, $i, @data, "#  Distributed under the Boost Software License, Version 1.0.\n"); ++$i;
    expect($path, $i, @data, "#  (See accompanying file LICENSE_1_0.txt or copy at\n"); ++$i;
    expect($path, $i, @data, "#  http://www.boost.org/LICENSE_1_0.txt)\n"); ++$i;
    expect($path, $i, @data, ('#'x76)."\n"); ++$i;
}

### Main ###

my @dirs_to_search = ();
my $disable_authors = 0;

my $expect_pattern = 0;
foreach my $arg (@ARGV) {
    if ($expect_pattern) {
      push(@dirs_to_search, $arg);
      $expect_pattern = 0;
      $disable_authors = 1;
    } elsif ($arg eq "-w") { $write_changes = 1; }
    elsif ($arg eq "-e") { $launch_emacs = 1; }
    elsif ($arg eq "-m") { $email_multimap = 1; }
    elsif ($arg eq "-f") { $expect_pattern = 1; }
    else {
        print "Unknown parameter: $arg\n";
    }
}

@dirs_to_search = ("doc", "examples", "include", "lib", "misc", "tests", "tools") unless @dirs_to_search;

(-e "include/stxxl.h")
    or die("Please run this script in the STXXL source base directory.");

# check uncrustify's version:
eval {
    no warnings;
    my $uncrustver = `uncrustify-0.64 --version` or die;
    if ($uncrustver eq "uncrustify 0.64\n") {
        $uncrustify = "uncrustify-0.64";
    }
};
if ($@) {
    my ($uncrustver) = filter_program("", "uncrustify", "--version");
    ($uncrustver eq "uncrustify 0.64\n")
        or die("Requires uncrustify 0.64 to run correctly. Got: $uncrustver");
    $uncrustify = "uncrustify";
}

use File::Find;
my @filelist;
find(sub { !-d && push(@filelist, $File::Find::name) }, @dirs_to_search);

foreach my $file (@filelist)
{
    if ($file =~ m!^extlib/!) {
        # skip external libraries
    }
    elsif ($file =~ /\.(h|cpp|h.in)$/) {
        process_cpp($file);
    }
    elsif ($file =~ m!^doc/[^/]*\.dox$!) {
        process_cpp($file);
    }
    elsif ($file =~ m!^include/stxxl/[^/]*$!) {
        process_cpp($file);
    }
    elsif ($file =~ m!^include/stxxl/[^/]*$!) {
        process_cpp($file);
    }
    elsif ($file =~ m!\.(pl|plot)$!) {
        process_pl_cmake($file);
    }
    elsif ($file =~ m!/CMakeLists\.txt$!) {
        process_pl_cmake($file);
    }
    # recognize further files
    elsif ($file =~ m!^[^/]*$!) { # files in source root
    }
    elsif ($file =~ m!^\.git/!) {
    }
    elsif ($file =~ m!^misc/!) {
    }
    elsif ($file =~ m!^doc/images/.*\.(png|pdf|svg|xmi)$!) {
    }
    elsif ($file =~ m!^doc/[^/]*\.(xml|css|bib)$!) {
    }
    elsif ($file =~ m!^doxygen-html!) {
    }
    elsif ($file =~ m!^lib/libstxxl\.symbols$!) {
    }
    elsif ($file =~ m!README$!) {
    }
    else {
        print "Unknown file type $file\n";
    }
}

# check include_map for C-style headers
{

    my @cheaders = qw(assert.h ctype.h errno.h fenv.h float.h inttypes.h
                      limits.h locale.h math.h signal.h stdarg.h stddef.h
                      stdlib.h stdio.h string.h time.h);

    foreach my $ch (@cheaders)
    {
        next if !$include_list{$ch};
        print "Replace c-style header $ch in\n";
        print "    [".join(",", sort keys %{$include_list{$ch}}). "]\n";
    }
}

# print includes to include_map.txt
if (0)
{
    print "Writing include_map:\n";
    foreach my $inc (sort keys %include_map)
    {
        print "$inc => ".scalar(keys %{$include_map{$inc}})." [";
        print join(",", sort keys %{$include_map{$inc}}). "]\n";
    }
}

# try to find cycles in includes
if (1)
{
    my %graph = %include_map;

    # Tarjan's Strongly Connected Components Algorithm
    # https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm

    my $index = 0;
    my @S = [];
    my %vi; # vertex info

    sub strongconnect {
        my ($v) = @_;

        # Set the depth index for v to the smallest unused index
        $vi{$v}{index} = $index;
        $vi{$v}{lowlink} = $index++;
        push(@S, $v);
        $vi{$v}{onStack} = 1;

        # Consider successors of v
        foreach my $w (keys %{$graph{$v}}) {
            if (!defined $vi{$w}{index}) {
                # Successor w has not yet been visited; recurse on it
                strongconnect($w);
                $vi{$w}{lowlink} = min($vi{$v}{lowlink}, $vi{$w}{lowlink});
            }
            elsif ($vi{$w}{onStack}) {
                # Successor w is in stack S and hence in the current SCC
                $vi{$v}{lowlink} = min($vi{$v}{lowlink}, $vi{$w}{index})
            }
        }

        # If v is a root node, pop the stack and generate an SCC
        if ($vi{$v}{lowlink} == $vi{$v}{index}) {
            # start a new strongly connected component
            my @SCC;
            my $w;
            do {
                $w = pop(@S);
                $vi{$w}{onStack} = 0;
                # add w to current strongly connected component
                push(@SCC, $w);
            } while ($w ne $v);
            # output the current strongly connected component (only if it is not
            # a singleton)
            if (@SCC != 1) {
                print "-------------------------------------------------";
                print "Found cycle:\n";
                print "    $_\n" foreach @SCC;
                print "end cycle\n";
            }
        }
    }

    foreach my $v (keys %graph) {
        next if defined $vi{$v}{index};
        strongconnect($v);
    }
}

# print authors to AUTHORS
if ($disable_authors) {
   print "Skip writing authors\n";
} else {
   print "Writing AUTHORS:\n";
   open(A, "> AUTHORS");
   foreach my $a (sort keys %authormap)
   {
       my $mail = $authormap{$a};
       if ($email_multimap) {
           $mail = join(",", sort keys %{$mail});
       }
       else {
           $mail = (sort keys(%{$mail}))[0]; # pick first
       }
       $mail = $mail ? " <$mail>" : "";
   
       print "  $a$mail\n";
       print A "$a$mail\n";
   }
   close(A);
}

# run cpplint.py
{
    my @lintlist;

    foreach my $path (@source_filelist)
    {
        #next if $path =~ /exclude/;

        push(@lintlist, $path);
    }

    system("cpplint", "--counting=total",
           "--extensions=h,c,cc,hpp,cpp", "--quiet", @lintlist);
}

################################################################################
