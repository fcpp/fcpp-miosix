#!/usr/bin/perl

use warnings;
use strict;

my $start=7*60; # Start time for statsing stats
my $stop=17*60; # Stop  time for statsing stats

my $lines=0;    # Number of lines between start and end time
my %stats;      # Neighbor statistics between start and end time

my %nodemapping =
(
    33115 => 0,
    33060 => 3,
    65449 => 5,
    65499 => 9,
    65496 => 10,
    65448 => 11,
    65498 => 12,
    65497 => 13
);

while(<>)
{
    next unless(/(\d+\.?\d*) \d+ \d \d \d \d \d \d+ \d+ \d+ \d+ \[(.*)\]/); 
    my $time=$1;
    next if($time<$start || $time>$stop);
    $lines++;
    my $neighbors=$2;
    $neighbors =~ s/,//g;
    foreach my $neighbor (split / /, $neighbors)
    {
        $stats{$neighbor}++;
    }
}

print "Stats for $lines lines\n";
foreach my $neighbor (keys %stats)
{
    my $stats=$stats{$neighbor};
    my $uptime=100*$stats/$lines;
    my $mappedNeigbor=$nodemapping{$neighbor};
    print "Neighbor $mappedNeigbor link uptime $uptime%\n";
}
