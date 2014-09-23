#!/usr/bin/perl -w

use strict;
use warnings;

if ( $#ARGV != 1 ) {
        die "$0 FILE1 FILE2\n";
}

my @records;
{
        my @lines = `diff -w $ARGV[0] $ARGV[1]`;
        chomp @lines;
        my $currentRecordLine;
        my $currentRecordRemoveLines;
        my $currentRecordAddLines;
        my $currentRecordStarted = 0;

        foreach (@lines) {
                if ( m/[0-9,\-]+[c|d|a][0-9,\-]+/g) {
                        if ( $currentRecordStarted ) {
                                push @records, { 'LinesText' => $currentRecordLine, 'RemoveLines' => $currentRecordRemoveLines, 'AddLines' => $currentRecordAddLines, };
                        }
                        $currentRecordLine = $_;
                        $currentRecordRemoveLines = [];
                        $currentRecordAddLines = [];
                        $currentRecordStarted = 1;
                } elsif ( m/^</g) {
                        push(@{$currentRecordRemoveLines}, $_);
                } elsif ( m/^>/g) {
                        push(@{$currentRecordAddLines}, $_);
                }
        }
}

my @matchRemove;
my @matchAdd;
my $Epsilon = 0.00000000000005;
foreach (@records) {
    my $printDiff = 1;

    if (scalar @{$_->{"RemoveLines"}} == scalar @{$_->{"AddLines"}}) {
        my $diffFound = scalar @{$_->{"RemoveLines"}};
        for (my $i = 0; $i < scalar @{$_->{"RemoveLines"}}; $i++) {
            if ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<HR>([0-9\.]+)<\/HR>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<HR>([0-9\.]+)<\/HR>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<Latitude>([\-0-9\.]+)<\/Latitude>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<Latitude>([\-0-9\.]+)<\/Latitude>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<Longitude>([\-0-9\.]+)<\/Longitude>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<Longitude>([\-0-9\.]+)<\/Longitude>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<Speed>([0-9\.]+)<\/Speed>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<Speed>([0-9\.]+)<\/Speed>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<VerticalSpeed>([\-0-9\.]+)<\/VerticalSpeed>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<VerticalSpeed>([\-0-9\.]+)<\/VerticalSpeed>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<GPSHeading>([\-0-9\.]+)<\/GPSHeading>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<GPSHeading>([\-0-9\.]+)<\/GPSHeading>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<GPSSpeed>([0-9\.]+)<\/GPSSpeed>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<GPSSpeed>([0-9\.]+)<\/GPSSpeed>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<GPSAltitude>([0-9\.]+)<\/GPSAltitude>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<GPSAltitude>([0-9\.]+)<\/GPSAltitude>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<EHPE>([0-9\.]+)<\/EHPE>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<EHPE>([0-9\.]+)<\/EHPE>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<Duration>([0-9\.]+)<\/Duration>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<Duration>([0-9\.]+)<\/Duration>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<Time>([0-9\.]+)<\/Time>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<Time>([0-9\.]+)<\/Time>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<EnergyConsumption>([0-9\.]+)<\/EnergyConsumption>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<EnergyConsumption>([0-9\.]+)<\/EnergyConsumption>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<PeakTrainingEffect>([0-9\.]+)<\/PeakTrainingEffect>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<PeakTrainingEffect>([0-9\.]+)<\/PeakTrainingEffect>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<BatteryCharge>([0-9\.]+)<\/BatteryCharge>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<BatteryCharge>([0-9\.]+)<\/BatteryCharge>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<BatteryChargeAtStart>([0-9\.]+)<\/BatteryChargeAtStart>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<BatteryChargeAtStart>([0-9\.]+)<\/BatteryChargeAtStart>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<Avg>([0-9\.]+)<\/Avg>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<Avg>([0-9\.]+)<\/Avg>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<Max>([0-9\.]+)<\/Max>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<Max>([0-9\.]+)<\/Max>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  $Epsilon ) {
                    $diffFound--;
                }
            }
            #elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<UTC>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:([0-9]{2})Z<\/UTC>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<UTC>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:([0-9]{2})Z<\/UTC>/g))) {
            #    if (($matchRemove[0] - $matchAdd[0]) <= 1 &&
            #        ($matchRemove[0] - $matchAdd[0]) >= -1) {
            #        $diffFound--;
            #    }
            #}
            elsif ((@matchRemove = (@{$_->{"RemoveLines"}}[$i] =~ m/<UTC>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:([0-9]{2}\.[0-9]{3})Z<\/UTC>/g)) && (@matchAdd = (@{$_->{"AddLines"}}[$i] =~ m/<UTC>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:([0-9]{2}\.[0-9]{3})Z<\/UTC>/g))) {
		if ( abs(($matchRemove[0] - $matchAdd[0])) <  0.002 ) {
                    $diffFound--;
                }
            }
        }
        if ($diffFound == 0) {
            $printDiff = 0;
        }
    }

    if ($printDiff == 1) {
        print $_->{"LinesText"}, "\n";
        foreach my $remLine (@{$_->{"RemoveLines"}}) {
            print $remLine, "\n";
        }
        if (scalar @{$_->{"RemoveLines"}} > 0 &&  scalar @{$_->{"AddLines"}} > 0) {
            print "---\n";
        }
        foreach my $remLine (@{$_->{"AddLines"}}) {
            print $remLine, "\n";
        }
    }
}


