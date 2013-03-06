#!/usr/bin/perl -w

## @file
# A quick and dirty script to convert .md files to .html files in a form useful
# for creating distributions of TWScript.

use v5.12;
use Text::MultiMarkdown qw(markdown);

## A simple html template for pages
my $htmltem = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<title>{title}</title>\n<link rel=\"stylesheet\" href=\"markdown.css\" type=\"text/css\" />\n</head>\n<body>\n{body}<div class=\"version\">{version}</div></body></html>";

## @fn void gen_doc($mdfile, $output, $version)
# Generate a html file from a markdown file. This loads the markdown file,
# parses it into html, and then saves the result to an output file wrapped
# in the html template.
#
# @param mdfile  The name of the markdown file to convert.
# @param output  The name of the html file to write.
# @param version The version string to set in the documentation.
sub gen_doc {
    my $mdfile  = shift;
    my $output  = shift;
    my $version = shift;

    # Pull the markdown text into memory
    local $/;
    open(MD, "<:utf8", $mdfile)
        or die "Unable to open '#$mdfile': $!\n";
    my $mdtext = <MD>;
    close(MD);

    # Convert it
    my $html = markdown($mdtext);

    # Obtain a title - this'll look for the first <h1> and use that
    my ($title) = $html =~ /<h1.*?>(.*?)<\/h1>/;
    $title = $mdfile if(!$title);

    # Shove the title and converted text into a html document
    my $outhtml = $htmltem;
    $outhtml =~ s/\{title\}/$title/g;
    $outhtml =~ s/\{body\}/$html/g;
    $outhtml =~ s/\{version\}/$version/g;

    # And save
    open(OUT, ">:utf8", $output)
        or die "Unable to open '$output': $!\n";
    print OUT $outhtml;
    close(OUT);
}

# The markdown filename is the first, required, parameter.
my $filename = $ARGV[0]
    or die "Usage: makedocs.pl <markdownfile> [<output>]";

# Pull out everything but the extension, in case an output name needs to be constructed
my ($base) = $filename =~ /^(.*?)\.\w+$/;
die "Unable to determine name from '$filename'\n" if(!$base);

# Work out the output name, falling back on the markdown filename + .html if not set
my $outfile = $ARGV[1] || $base.".html";

# Now work out the version string
my $gitvers = `git tag -n1 | sort -V | tail -n1`;
$gitvers =~ s/^.*?\s\s+(.*)$/$1/; # Strip the lead tag, leaving just the annotation.

gen_doc($filename, $outfile, $gitvers);
