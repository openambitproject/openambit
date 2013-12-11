#!/usr/bin/perl

#use strict;
#use warnings;

my $diffText = `diff -w $ARGV[0] $ARGV[1]`;

my @lines = split /\n/, $diffText;

my @records;
my $currentRecordLine;
my $currentRecordRemoveLines;
my $currentRecordAddLines;
my $currentRecordStarted = 0;

foreach my $line (@lines) {
    if ($line =~ m/[0-9,\-]+[c|d|a][0-9,\-]+/g) {
        if ($currentRecordStarted == 1) {
            my %currentRecord = ( "LinesText" => $currentRecordLine,
                                  "RemoveLines" => $currentRecordRemoveLines,
                                  "AddLines" => $currentRecordAddLines
                );
            push(@records, \%currentRecord);
        }
        $currentRecordLine = $line;
        $currentRecordRemoveLines = [];
        $currentRecordAddLines = [];
        $currentRecordStarted = 1;
    }
    elsif ($line =~ m/^</g) {
        push(@{$currentRecordRemoveLines}, $line);
    }
    elsif ($line =~ m/^>/g) {
        push(@{$currentRecordAddLines}, $line);
    }
}

foreach my $entry (@records) {
    my $printDiff = 1;

    if (scalar @{$entry->{"RemoveLines"}} == scalar @{$entry->{"AddLines"}}) {
        my $diffFound = scalar @{$entry->{"RemoveLines"}};
        for ($i = 0; $i < scalar @{$entry->{"RemoveLines"}}; $i++) {
            if ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<HR>([0-9\.]+)<\/HR>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<HR>([0-9\.]+)<\/HR>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<Latitude>([\-0-9\.]+)<\/Latitude>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<Latitude>([\-0-9\.]+)<\/Latitude>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.000000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.000000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<Longitude>([\-0-9\.]+)<\/Longitude>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<Longitude>([\-0-9\.]+)<\/Longitude>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.000000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.000000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<Speed>([0-9\.]+)<\/Speed>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<Speed>([0-9\.]+)<\/Speed>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<VerticalSpeed>([\-0-9\.]+)<\/VerticalSpeed>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<VerticalSpeed>([\-0-9\.]+)<\/VerticalSpeed>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<GPSHeading>([\-0-9\.]+)<\/GPSHeading>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<GPSHeading>([\-0-9\.]+)<\/GPSHeading>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<GPSSpeed>([0-9\.]+)<\/GPSSpeed>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<GPSSpeed>([0-9\.]+)<\/GPSSpeed>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<GPSAltitude>([0-9\.]+)<\/GPSAltitude>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<GPSAltitude>([0-9\.]+)<\/GPSAltitude>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<EHPE>([0-9\.]+)<\/EHPE>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<EHPE>([0-9\.]+)<\/EHPE>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<Duration>([0-9\.]+)<\/Duration>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<Duration>([0-9\.]+)<\/Duration>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<Time>([0-9\.]+)<\/Time>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<Time>([0-9\.]+)<\/Time>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<EnergyConsumption>([0-9\.]+)<\/EnergyConsumption>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<EnergyConsumption>([0-9\.]+)<\/EnergyConsumption>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<PeakTrainingEffect>([0-9\.]+)<\/PeakTrainingEffect>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<PeakTrainingEffect>([0-9\.]+)<\/PeakTrainingEffect>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<BatteryCharge>([0-9\.]+)<\/BatteryCharge>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<BatteryCharge>([0-9\.]+)<\/BatteryCharge>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<BatteryChargeAtStart>([0-9\.]+)<\/BatteryChargeAtStart>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<BatteryChargeAtStart>([0-9\.]+)<\/BatteryChargeAtStart>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<Avg>([0-9\.]+)<\/Avg>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<Avg>([0-9\.]+)<\/Avg>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<Max>([0-9\.]+)<\/Max>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<Max>([0-9\.]+)<\/Max>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.00000000000005 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.00000000000005) {
                    $diffFound--;
                }
            }
            #elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<UTC>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:([0-9]{2})Z<\/UTC>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<UTC>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:([0-9]{2})Z<\/UTC>/g))) {
            #    if ((@matchRemove[0] - @matchAdd[0]) <= 1 &&
            #        (@matchRemove[0] - @matchAdd[0]) >= -1) {
            #        $diffFound--;
            #    }
            #}
            elsif ((@matchRemove = (@{$entry->{"RemoveLines"}}[$i] =~ m/<UTC>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:([0-9]{2}\.[0-9]{3})Z<\/UTC>/g)) && (@matchAdd = (@{$entry->{"AddLines"}}[$i] =~ m/<UTC>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:([0-9]{2}\.[0-9]{3})Z<\/UTC>/g))) {
                if ((@matchRemove[0] - @matchAdd[0]) < 0.002 &&
                    (@matchRemove[0] - @matchAdd[0]) > -0.002) {
                    $diffFound--;
                }
            }
        }
        if ($diffFound == 0) {
            $printDiff = 0;
        }
    }

    if ($printDiff == 1) {
        print $entry->{"LinesText"}, "\n";
        foreach my $remLine (@{$entry->{"RemoveLines"}}) {
            print $remLine, "\n";
        }
        if (scalar @{$entry->{"RemoveLines"}} > 0 &&  scalar @{$entry->{"AddLines"}} > 0) {
            print "---\n";
        }
        foreach my $remLine (@{$entry->{"AddLines"}}) {
            print $remLine, "\n";
        }
    }
}


