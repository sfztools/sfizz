#!/usr/bin/env perl
use strict;
use warnings;
use autodie;
use DateTime;

if (@ARGV != 1) {
    die qq{Usage: $0 "New Post Title".\n}
}

# Get the kind of post to create from the script name.
(my $what = $0) =~ s{^.*/new_(\w+)$}{$1};
my $dir   = '_posts';
my $title = $ARGV[0];

(my $name = $title) =~ tr/A-Z /a-z-/;
my $date = DateTime->now()->ymd;

open(my $fh, '>', "$dir/$date-$name.md");
binmode($fh);
print $fh <<EOF
---
title: "$title"
date: $date
comments: true
---
EOF
