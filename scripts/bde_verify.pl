#!/usr/bin/env perl
# bde_verify.pl                                                      -*-perl-*-

# Bde_verify is a static analysis tool that verifies that source code adheres
# to the BDE coding standards.  This version of the wrapper script is for
# Microsoft Windows.

use strict;
use warnings;
use Getopt::Long;
use Cwd;

my $bb       = $ENV{BDE_ROOT};
$bb          = '' unless -d $bb;

my $me       = Cwd::abs_path(${0});
my $pt       = $me;
$pt          =~ s{(bin/|scripts/)?[^/]*$}{};
my $nm       = ${0};
$nm          =~ s{.*[/\\]}{};

my $config   = "${pt}etc/bde-verify/${nm}.cfg";
$config      = "${pt}${nm}.cfg"                    unless -r $config;

my $enm      = "bde_verify_bin";
my $exe      = "${pt}libexec/bde-verify/$enm.exe";
$exe         = "${pt}$enm.exe"                     unless -x $exe;
$exe         = "$enm.exe"                          unless -x $exe;

my @cl;
my $debug    = "";
my $verbose  = "";
my $help     = "";
my $definc   = 1;
my $defdef   = 1;
my @incs;
my @defs;
my @wflags;
my @lflags = (
    "cxx-exceptions",
    "exceptions",
    "diagnostics-show-note-include-stack",
    "diagnostics-show-option",
    "error-limit=0",
    "ms-compatibility",
    "ms-extensions",
   #"msc-version=1800",
   #"delayed-template-parsing",
);
my $dummy;
my @dummy;
my $std;
my @std;
my $warnoff;
my $diagnose = "component";
my $rwd;
my $rwf;
my $tag;
my $ovr = -1;
my $m32;
my $m64;

my $command = join " \\\n ", $0, map { join '" "', split(/ /, $_, -1) } @ARGV;

sub usage()
{
    print "
usage: $0 [options] [additional compiler options] file.cpp ...
    -I{directory}
    -D{macro}
    -w                       # disable normal compiler warnings
    -W{warning}
    -f{flag}
    --config=config_file     [$config]  
    --cl=config_line
    --bb=dir                 [$bb]
    --exe=binary             [$exe]
    --[no]definc             [$definc]
    --[no]defdef             [$defdef]
    --[no]ovr                # whether to define BSL_OVERRIDES_STD
    --rewrite-dir=dir
    --rewrite-file=file
    --diagnose={main,component,nogen,all}
    --std=type
    --tag=string
    --debug
    --verbose
    --help

For full documentation, see {TEAM BDE:BDE_VERIFY<GO>} or
<http://cms.prod.bloomberg.com/team/display/bde/bde_verify>.

Invoked as:
$command
";
    exit(1);
}

@ARGV = map { /^(-+[DIOWf])(.+)/ ? ($1, $2) : $_ } @ARGV;

GetOptions(
    'bb=s'                         => \$bb,
    'config=s'                     => \$config,
    'cl=s'                         => \@cl,
    'debug'                        => \$debug,
    'help|?'                       => \$help,
    'cc=s'                         => \$dummy,
    'exe=s'                        => \$exe,
    'verbose|v'                    => \$verbose,
    'definc!'                      => \$definc,
    'defdef!'                      => \$defdef,
    'ovr!'                         => \$ovr,
    'nsa!'                         => \$dummy,
    'rewrite|rewrite-dir|rd=s'     => \$rwd,
    'rewrite-file|rf=s'            => \$rwf,
    'w'                            => \$warnoff,
    "I=s"                          => \@incs,
    "D=s"                          => \@defs,
    "W=s"                          => \@wflags,
    "f=s"                          => \@lflags,
    "std=s"                        => \$std,
    "toplevel"                     => sub { $diagnose = "main" },
    "diagnose=s"                   => sub {
        my %opts = ("main" => 1, "component" => 1, "nogen" => 1, "all" => 1);
        die "Unknown value '$_[1]' for '-$_[0]='\n" unless $opts{$_[1]};
        $diagnose = $_[1];
    },
    'tag=s'                        => \$tag,
    'llvm=s'                       => \$dummy,
    "m32"                          => \$m32,
    "m64"                          => \$m64,
    "pipe|pthread|MMD|g|c|S"       => \$dummy,
    "O|MF|o|march|mtune=s"         => \@dummy,
) and !$help and $#ARGV >= 0 or usage();

sub xclang(@) { return map { ( "-Xclang", $_ ) } @_; }
sub plugin(@) { return xclang( "-plugin-arg-bde_verify", @_ ); }

my @config = plugin("config=$config")      if $config ne "";
my @debug  = plugin("debug-on")            if $debug;
my @tlo    = plugin("diagnose=$diagnose");
my @rwd    = plugin("rewrite-dir=$rwd")    if $rwd;
print "No such directory $rwd\n" if $rwd and ! -d $rwd;
my @rwf    = plugin("rewrite-file=$rwf")   if $rwf;
my @tag    = plugin("tool=$tag")           if $tag;
my %uf     = (
             );
@lflags = map { "-f$_" } grep { not exists $uf{$_} } @lflags;
@wflags = map { "-W$_" } grep { not /,/ } @wflags;
unshift @wflags, "-w" if $warnoff;

my $macro = ${nm};
$macro =~ s{^([^.]*).*}{\U$1\E};
push(@defs, ($macro));
push(@defs, (
    "BB_THREADED",
    "BDE_BUILD_TARGET_DBG",
    "BDE_BUILD_TARGET_EXC",
    "BDE_BUILD_TARGET_MT",
    "BDE_DCL=",
    "BDE_OMIT_INTERNAL_DEPRECATED",
    "BSLS_IDENT_OFF",
    "BSL_DCL=",
    "DEBUG",
    "NOGDI",
    "NOMINMAX",
    "WINVER=0x0500",
    "_CRT_SECURE_NO_DEPRECATE",
    "_MT",
    "_SCL_SECURE_NO_DEPRECATE",
    "_STLP_HAS_NATIVE_FLOAT_ABS",
    "_STLP_USE_STATIC_LIB",
    "_WIN32_WINNT=0x0500",
)) if $defdef;
if ($ovr == 1 || $ovr != 0 && $defdef && !grep(m{\bb[sd]l[^/]*$}, @ARGV)) {
    push(@defs, "BSL_OVERRIDES_STD");
}
@defs = map { "-D$_" } @defs;
if ($ovr == 0) {
    push(@defs, "-UBSL_OVERRIDES_STD");
}

for (@ARGV) {
    if (! -f) {
        warn "Cannot find file $_\n";
    } elsif (m{^(.*)[/\\].*$}) {
        push(@incs, $1) unless grep($_ eq $1, @incs);
    } else {
        push(@incs, ".") unless grep($_ eq ".", @incs);
    }
}
push(@incs, (
    "$bb/include",
    "$bb/include/bsl+stdhdrs",
)) if $bb and $definc;
for (@incs) { warn "cannot find directory $_\n" unless -d; }
@incs = map { ( "-I", $_ ) }
        map { $_ eq Cwd::cwd() ? "." : $_ }
        map { Cwd::abs_path($_) }
        grep { -d } @incs;

my @pass;
push(@pass, "-m32") if $m32;
push(@pass, "-m64") if $m64;

my $inc = $ENV{INCLUDE} ||
    "C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/INCLUDE;" .
    "C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/ATLMFC/INCLUDE;" .
    "C:/Program Files (x86)/Windows Kits/8.1/include/shared;" .
    "C:/Program Files (x86)/Windows Kits/8.1/include/um;" .
    "C:/Program Files (x86)/Windows Kits/8.1/include/winrt";
push(@incs, map { ( "-isystem", $_ ) }
            map { Cwd::abs_path($_) }
            grep { -d } split(/;/, $inc));

push(@std, "-std=$std") if $std;

@cl = map { plugin("config-line=$_") } @cl;

my @command = (
    "$exe",
    xclang("-plugin", "bde_verify"),
    "-resource-dir", "${pt}include/bde-verify/clang",
    "-msoft-float",
    "-fsyntax-only",
    "-xc++",
    @std,
    @debug,
    @config,
    @tlo,
    @rwd,
    @rwf,
    @tag,
    @cl,
    @defs,
    @incs,
    @lflags,
    @wflags,
    @ARGV,
);

map { s{\\}{/}g } @command;

if ($verbose)
{
    my @c = @command;
    map { s{(.*)}{"$1"} if m{[( )]} and not m{"} } @c;
    print join(" \\\n ", @c), "\n";
}

exec @command;

## ----------------------------------------------------------------------------
## Copyright (C) 2014 Bloomberg Finance L.P.
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to
## deal in the Software without restriction, including without limitation the
## rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
## sell copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in
## all copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
## FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
## IN THE SOFTWARE.
## ----------------------------- END-OF-FILE ----------------------------------
