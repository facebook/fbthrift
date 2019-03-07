#!/usr/bin/perl -w

use strict;

my $cmd = "git log HEAD^..HEAD --date=iso ";
my $out = `$cmd`;
$out =~ s/\n/ /sg;
my $string = $out;

if ($string =~ /commit\s+(\S{6}).*?Date:\s+(\d+-\d+-\d+) (\d+:\d+:\d+)/) {
  my ($commit, $day, $time) = ($1, $2, $3);
  $time =~ s/:/\./g;
  
  print "${day}T$time-$commit\n";
}
