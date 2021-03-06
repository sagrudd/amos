#!/usr/local/bin/perl

# $Id$
#
# printScaff.pl takes in the output of grommit together with the input
# to grommit to generate various reports such as simple summary information
# or .dot input to the GraphViz package.

#  Copyright @ 2002, 2003 The Institute for Genomic Research (TIGR).  All
#  rights reserved.

use strict;

use XML::Parser;
use TIGR::Foundation;
use TIGR::FASTAreader;
use TIGR::FASTArecord;

use AMOS::DotLib;
use TIGR::AsmLib;

my $base = new TIGR::Foundation;

if (! defined $base){
    die ("Walk, do not run to the nearest exit!");
}

my $VERSION = "1.01";
my $VERSION_STRING = "$VERSION (" . '$Revision$ ' . ")";

$base->setVersionInfo($VERSION_STRING);

my $HELPTEXT = q~
    Report statistics about a scaffold

    printScaff -e <evidencefile> -s <scaffile> -o <outprefix>
               -l <libconf> [-dot -page -plot -unused -phys -oo -sum] [-detail]
               -f <fasta_in> [-[no]merge] [-arachne <araprefix>]
    ~;

$base->setHelpInfo($HELPTEXT);

my $evidencefile;
my $scaffoldfile;
my $outprefix;
my $dodot;
my $page;
my $plot;
my $detail;
my $unused;
my $libconf;
my $physicals;  # list particular edges as previously physical gaps
my $fastain;    # fasta file of all contigs
my $oo;         # print object and orientation file
my $sum;        # print summary file
my $merge = 1;  # whether to merge the scaffold extents
my $arachne;    # prefix for arachne outputs

my $fastaSep;  # separator of contigs in fasta file
for (my $i = 0; $i < 60; $i++){
    $fastaSep .= "N";
}

my $err = $base->TIGR_GetOptions("e=s"       => \$evidencefile,
				 "s=s"       => \$scaffoldfile,
				 "o=s"       => \$outprefix,
				 "l=s"       => \$libconf,
				 "dot"       => \$dodot,
				 "page"      => \$page,
				 "plot"      => \$plot,
				 "detail"    => \$detail,
				 "unused"    => \$unused,
				 "phys=s"    => \$physicals,
				 "f=s"       => \$fastain,
                                 "oo"        => \$oo,
				 "sum"       => \$sum,
                                 "merge!"    => \$merge,
				 "arachne=s" => \$arachne
				 );

$base->bail("Option processing failed") unless $err;

if ((defined $page || defined $plot || defined $unused || defined $physicals) 
    && ! defined $dodot){
    $base->bail("Options -page, -plot, -phys, and -unused require option -dot");
}

if (! defined $evidencefile){
    $base->bail("An evidence file must be provided with option -e");
}

if (! defined $scaffoldfile){
    $base->bail("A scaffold file must be provided with option -s");
}

if (! defined $outprefix){
    $base->bail("An output prefix must be provided with option -o");
}

if (! defined $libconf){
    $base->bail("A library file must be provided with option -l");
}

my $fr;
my @errors;
if (defined $fastain){
    $fr = new TIGR::FASTAreader($base, \@errors, $fastain);
    if (! defined $fr){
	$base->bail("Cannot open $fastain: bad FASTA reader\n");
    }
}

my $dotname    = $outprefix . ".dot";
my $statname   = $outprefix . ".stats";
my $detailname = $outprefix . ".details";
my $physname   = $outprefix . ".phys";
my $fastaout   = $outprefix . ".fasta";
my $ooout      = $outprefix . ".oo";
my $sumout     = $outprefix . ".sum";

my $traceinfo  = $arachne   . ".xml";
my $aralinks   = $arachne   . ".links";
my $arareads   = $arachne   . ".reads";
my $arabases   = $arachne   . ".bases"; # requires a .fasta file

# take care of parsing the XML
my $xml = new XML::Parser(Style => 'Stream');

my $inInsert;
my $insId;
my $conId;
my $libId;
my %inserts;
my %seqName;
my %contigs;
my %seqCtg;
my %seqOri;
my %seqLend;
my %seqRend;
my %libraries;
my %insertLib;
my $inLib = 0;
my $inLink = 0;
my $inContig = 0;
my %linkCtg;
my %conEv;
my %libClass;
my $inUnused = 0;
my $scaffId;
my %contigScaff;  # contig 2 scaffold
my %scaffContig;  # scaffold 2 contig
my @unseen;
my %linkValid;
my %contigStart;
my %contigEnd;
my %contigX;
my %contigOri;
my %scaffLink;
my @scaffolds; # list of all scaffolds
my %scaffSize; # sum of contigs in scaffold
my $id;
my $doingUnused = 0;  # flag for printDetails

open(LIB, $libconf) ||
    $base->bail("Cannot open lib file \"$libconf\": $!");

while (<LIB>){
    chomp;
    my @fields = split(' ', $_);
    for (my $i = 1; $i <= $#fields; $i++){
	$libClass{$fields[$i]} = $fields[0];
    }
}
close(LIB);

my %physicals;

if (defined $physicals){
    open(PHY, ">$physname") ||
	$base->bail("Cannot open physical gap file \"$physname\": $!");
    my @phs = split(',', $physicals);
    for (my $i = 0; $i <= $#phs; $i++){
	$physicals{$libClass{$phs[$i]}} = 1;
    }
}

my $inEvidence;

$xml->parsefile($evidencefile);
$xml->parsefile($scaffoldfile);

# sort scaffolds by size
@scaffolds = sort {$scaffSize{$b} <=> $scaffSize{$a}} @scaffolds;

if (defined $dodot){
    open(DOT, ">$dotname") ||
	$base->bail("Cannot open dot file \"$dotname\": $!");
    if (defined $plot) {
	printHeader(\*DOT, "plotter");
    } elsif (defined $page) {
	printHeader(\*DOT, "printer");
    } else {
	printHeader(\*DOT, undef);
    }
}

if (defined $oo){
    open(OO, ">$ooout") ||
	$base->bail("Cannot open OO file \"$ooout\": $!");
}

if (defined $sum){
    open(SUM, ">$sumout") ||
	$base->bail("Cannot open SUM file \"$sumout\": $!");
}

my @scaffSpan;
my @scaffContigs;
my $scSz = 0;
my $scSp = 0;
my $scCtg = 0;

if (defined $detail){
    open(DETAIL, ">$detailname") ||
	$base->bail("Cannot open \"$detailname\": $!");
}

my %validLs = ();
my %invalidLens = ();
my %invalidOris =();
my %valids = ();
my %invalidLen = ();
my %invalidOri = ();

if (defined $fastain){
    open(FASTA, ">$fastaout") ||
	$base->bail("Cannot open \"$fastaout\": $!");
}

for (my $i = 0; $i <= $#scaffolds; $i++){
    my $scaff = $scaffolds[$i];
    my @ctgs = split(' ', $scaffContig{$scaff});
    my @ctgends = @ctgs;
    
    @ctgs = sort {$contigStart{$a} <=> $contigStart{$b}} @ctgs;
    @ctgends = sort {$contigEnd{$a} <=> $contigEnd{$b}} @ctgends;
    my $nctgs = $#ctgs + 1;

    # Two output formats are possible here.  First is default in
    # which the consituent contigs are merged using a string of N's,
    # with the correct orientation.  The second (if the -nomerge
    # option is specified) prints the contig records with the
    # scaffoldId_scaffoldSubid_contig_contigId format.
    #
    if (defined $fastain){
	if ($merge){
	    my $scaffSeq = "";
	    for (my $c = 0; $c <= $#ctgs; $c++){
		my $ctgid = $ctgs[$c];
		$ctgid =~ s/^contig_(\S+)/$1/;
		my $rec = $fr->getRecordByIdentifier($ctgid);
		if (! defined $rec){
		    $base->logError("cannot find fasta record for contig $ctgid\n");
		    next;
		}
		my $thisctg = $rec->getData();
		if ($contigOri{$ctgs[$c]} eq "EB"){
		    $thisctg = reverseComplement($thisctg);
		}
		if ($scaffSeq eq ""){
		    $scaffSeq = $thisctg;
		} else {
		    $scaffSeq .= $fastaSep;
		    $scaffSeq .= $thisctg;
		}
	    }
	    printFastaSequence(\*FASTA, "$scaff", $scaffSeq);
	} else { # if no merge
	    my $scaffSeq = "";
	    for (my $c = 0; $c <= $#ctgs; $c++)
	    {
		my $ctgid = $ctgs[$c];
		$ctgid =~ s/^contig_(\S+)/$1/;
		my $rec = $fr->getRecordByIdentifier($ctgid);
		if (! defined $rec)
		{
		    $base->bail("cannot find fasta record for contig $ctgid\n");
		}
		my $contig_rec = ($contigOri{$ctgs[$c]} eq "EB")?  
		    "$scaff" . "_contig_" . $ctgid . "_EB" : 
		    "$scaff" . "_contig_" . $ctgid . "_BE" ;
		my $thisctg = $rec->getData();
		if ($contigOri{$ctgs[$c]} eq "EB")
		{
		    $thisctg = reverseComplement($thisctg);
		}
		printFastaSequence(\*FASTA, "$contig_rec", $thisctg);
	    } # for each contig
	} # if no merge
    } # if defined fastain
    my $maxSpan = $contigEnd{$ctgs[0]};

    for (my $c = 1; $c <= $#ctgs; $c++){
	if ($contigEnd{$ctgs[$c]} > $maxSpan){
	    $maxSpan = $contigEnd{$ctgs[$c]};
	}
    }

    my $span = $maxSpan - $contigStart{$ctgs[0]};

    if (defined $dodot){
	startCluster(\*DOT, $scaff, "Scaffold $scaff n:$nctgs size:$scaffSize{$scaff} span:$span");
    }

    if (defined $sum){
	print SUM "$scaff\t$nctgs\t$scaffSize{$scaff}\t$span\n";
    }
    if (defined $oo){
	print OO ">$scaff $nctgs $scaffSize{$scaff} $span\n";
    }

    push(@scaffSpan, $span);
    push(@scaffContigs, $nctgs);

    if (defined $detail){
	print DETAIL "Scaffold $scaff $nctgs contigs $scaffSize{$scaff} bases $span span\n";
	print DETAIL "\n";
    }

    my $gotTo = $contigEnd{$ctgs[0]};
    my $lastCtg = $ctgs[0];
    my $last = 0;

    for (my $c = 0; $c <= $#ctgs; $c++){
	my $ctg = $ctgs[$c];
	my $ori = ($contigOri{$ctg} eq "BE") ? 1 : -1;

	if (defined $oo){
	    print OO "$ctg $contigOri{$ctg}\n";
	}

	if (defined $dodot){
	    printNode(\*DOT, $ctg, "$ctg ($contigs{$ctg})\\n$contigStart{$ctg}       $contigEnd{$ctg}", $ori);
	    
	    # add invisible edge between two contigs if they don't
	    # overlap
	    while ($last <= $#ctgends && $contigEnd{$ctgends[$last]} - 1 < $contigStart{$ctg}){
		$last++;
	    }
	    if ($last > 0){
		printEdge(\*DOT, $ctgends[$last - 1], $ctg, "", "invis");
	    }
	}
	if ($contigEnd{$ctg} > $gotTo){
	    $gotTo = $contigEnd{$ctg};
	    $lastCtg = $ctg;
	}
    } # for my $c....
    
    my @links = split(' ', $scaffLink{$scaff});
    %valids = ();
    %invalidLen = ();
    %invalidOri = ();

    %validLs = ();
    %invalidLens = ();
    %invalidOris =();

    my %edgeLib = ();

    for (my $l = 0; $l <= $#links; $l++){
	my $lnk = $links[$l];
	
	my ($ctgA, $sa, $ctgB, $sb) = ctgPair($lnk);

	if ($sa != $scaff || $sb != $scaff){
	    $base->logError("Wierd link $lnk in scaffold $scaff connects two scaffolds $sa and $sb", 1);
	    next;
	}

	my $edge = ($contigStart{$ctgA} <= $contigStart{$ctgB}) ? "$ctgA $ctgB" : "$ctgB $ctgA";

	if ($linkValid{$lnk} eq "VALID"){
	    $valids{$edge}++;
	    $validLs{$edge} .= " $lnk";
	    $edgeLib{$edge}{$libClass{$insertLib{$lnk}}}++;
	} elsif ($linkValid{$lnk} eq "LEN"){
	    $invalidLen{$edge}++;
	    $invalidLens{$edge} .= " $lnk";
	} elsif ($linkValid{$lnk} eq "ORI"){
	    $invalidOri{$edge}++;
	    $invalidOris{$edge} .= " $lnk";
	}
    }

    while (my ($e, $n) = each %valids){
	my ($ctgA, $ctgB) = split(' ', $e);
	my $lens;
	my $oris;
	if (exists $invalidLen{$e}){
	    $lens = $invalidLen{$e};
	    delete $invalidLen{$e};
	} 
	if (exists $invalidOri{$e}){
	    $oris = $invalidOri{$e};
	    delete $invalidOri{$e};
	}

#	my $libinfo = getLibInfo($e);

	if (defined $dodot){
	    my $libdata;
	    my $isPhys = 1;
	    while (my ($l, $nn) = each %{$edgeLib{$e}}){
		$libdata .= "$l:$nn ";
		if (defined $physicals && ! exists $physicals{$l}){
		    $isPhys = 0;
		}
	    }
	    printEdge(\*DOT, $ctgA, $ctgB, "$n\\n$libdata\\nL:$lens    O:$oris", undef);
	    if (defined $physicals && $isPhys){
		print PHY "$ctgA $contigOri{$ctgA} $ctgB $contigOri{$ctgB} ",
		$contigStart{$ctgB} - $contigEnd{$ctgA}, "\n";
	    }
	}
	if (defined $detail){
	    printDetail(\*DETAIL, $e);
	}
    }

    while (my ($e, $n) = each %invalidLen){
	my ($ctgA, $ctgB) = split(' ', $e);
	my $oris;
	if (exists $invalidOri{$e}){
	    $oris = $invalidOri{$e};
	    delete $invalidOri{$e};
	}
	if (defined $dodot){
	    printEdge(\*DOT, $ctgA, $ctgB, "L:$n     O:$oris", "dashed");
	}
	if (defined $detail) {
	    printDetail(\*DETAIL, $e);
	}
    }
    while (my ($e, $n) = each %invalidOri){
	my ($ctgA, $ctgB) = split(' ', $e);
	if (defined $dodot){
	    printEdge(\*DOT, $ctgA, $ctgB, "L:0      O:$n", "dashed");
	}
	if (defined $detail) {
	    printDetail(\*DETAIL, $e);
	}
    }
    
    if (defined $dodot){
	endCluster(\*DOT);
    }
} # for each scaffold

$doingUnused = 1;

if (defined $detail){
    %valids = ();
    %validLs = ();
    %invalidLen = ();
    %invalidLens = ();
    %invalidOri = ();
    %invalidOris = ();
    for (my $l = 0; $l <= $#unseen; $l++){
	my $lnk = $unseen[$l];
	
	my ($ctgA, $sa, $ctgB, $sb) = ctgPair($lnk);
	my $edge = ($contigStart{$ctgA} <= $contigStart{$ctgB}) ? "$ctgA $ctgB" : "$ctgB $ctgA";
	
	if ($linkValid{$lnk} eq "VALID"){
	    $valids{$edge}++;
	    $validLs{$edge} .= " $lnk";
#	    $edgeLib{$edge}{$libClass{$insertLib{$lnk}}}++;
	} elsif ($linkValid{$lnk} eq "LEN"){
	    $invalidLen{$edge}++;
	    $invalidLens{$edge} .= " $lnk";
	} elsif ($linkValid{$lnk} eq "ORI"){
	    $invalidOri{$edge}++;
	    $invalidOris{$edge} .= " $lnk";
	}
    }
    print DETAIL "\n\n UNUSED LINKS:\n\n";

    while (my ($e, $n) = each %valids){
	my ($ctgA, $ctgB) = split(' ', $e);
	if (exists $invalidLen{$e}){
	    delete $invalidLen{$e};
	} 
	if (exists $invalidOri{$e}){
	    delete $invalidOri{$e};
	}
	printDetail(\*DETAIL, $e);
    }
    while (my ($e, $n) = each %invalidLen){
	my ($ctgA, $ctgB) = split(' ', $e);
	if (exists $invalidOri{$e}){
	    delete $invalidOri{$e};
	}
	printDetail(\*DETAIL, $e);
    }
    while (my ($e, $n) = each %invalidOri){
	my ($ctgA, $ctgB) = split(' ', $e);
	printDetail(\*DETAIL, $e);
    }
}

if (defined $dodot){
    printFooter(\*DOT);
    close(DOT);
}

if (defined $detail){
    close(DETAIL);
}

if (defined $fastain){
    close(FASTA);
}

if (defined $oo){
    close(OO);
}

if (defined $sum){
    close(SUM);
}

# now we should have contig stats

# starting to compute link stats
my %validLib;
my %lenLib;
my %oriLib;
my %unusedLib;
my $validL = 0; 
my $lenL = 0;
my $oriL = 0; 
my $unusedL = 0;
while (my ($lnk, $valid) = each %linkValid){
    my $lib = $insertLib{$lnk};
    if (! defined $lib){ 
	$base->logError("Cannot find library for insert $lnk\n");
	next;
    }

    if ($valid eq "VALID"){
	$validLib{$lib}++;
	$validL++;
    } elsif ($valid eq "LEN"){
	$lenLib{$lib}++;
	$lenL++;
    } elsif ($valid eq "ORI"){
	$oriLib{$lib}++;
	$oriL++;
    } elsif ($valid eq "UNSEEN"){
	$unusedLib{$lib}++;
	$unusedL++;
    }
}

my $genomeSize;
if (-e "genome.size"){
    open(GEN, "genome.size") || $base->logError("Cannot open genome.size: $!\n");
    while (<GEN>){
	if (/^(\d+)$/){
	    $genomeSize = $1;
	}
    }
    close(GEN);
}

# Here we print overall statistics
open(STAT, ">$statname") ||
    $base->bail("Cannot open stat file \"$statname\": $!");

@scaffSpan = sort {$b <=> $a} @scaffSpan;
@scaffContigs = sort {$b <=> $a} @scaffContigs;
my @scaffSz = sort {$b <=> $a} (values %scaffSize);


for (my $i = 0; $i <= $#scaffSpan; $i++){
    $scSp += $scaffSpan[$i];
}

for (my $i = 0; $i <= $#scaffContigs; $i++){
    $scCtg += $scaffContigs[$i];
}

for (my $i = 0; $i <= $#scaffSz; $i++){
    $scSz += $scaffSz[$i];
}

if (! defined $genomeSize){
    $genomeSize = $scSp;
}

my $n50Span = N50(\@scaffSpan, $genomeSize);
my $n50Size = N50(\@scaffSz, $genomeSize);

$base->logLocal("Assuming genome size of $genomeSize bases\n", 1);

print STAT "Genome size:\t$genomeSize\n\n";
print STAT "no. scaffolds:\t", $#scaffolds + 1, "\n";
print STAT "total scaffold size:\t", $scSz, "\n";
print STAT "min scaffold size:\t", $scaffSz[$#scaffSz], "\n";
print STAT "max scaffold size:\t", $scaffSz[0], "\n";
print STAT "avg scaffold size:\t", 
    sprintf("%.2f", 1.0 * $scSz / ($#scaffolds + 1)), "\n";
print STAT "N50 scaffold size:\t$n50Size\n";
print STAT "\n";

print STAT "total scaffold span:\t", $scSp, "\n";
print STAT "min scaffold span:\t", $scaffSpan[$#scaffSpan], "\n";
print STAT "max scaffold span:\t", $scaffSpan[0], "\n";
print STAT "avg scaffold span:\t", 
    sprintf("%.2f", 1.0 * $scSp / ($#scaffolds + 1)), "\n";
print STAT "N50 scaffold span:\t$n50Span\n";
print STAT "\n";

print STAT "total # contigs:\t", $scCtg, "\n";
print STAT "min # contigs:\t", $scaffContigs[$#scaffContigs], "\n";
print STAT "max # contigs:\t", $scaffContigs[0], "\n";
print STAT "avg # contigs:\t", 
    sprintf("%.2f", 1.0 * $scCtg / ($#scaffolds + 1)), "\n";
print STAT "\n";

print STAT "no. valid links:\t$validL\n";
print STAT "no. incorrect len. links:\t$lenL\n";
print STAT "no. incorrect ori. links:\t$oriL\n";
print STAT "no. unchecked links:\t$unusedL\n";
print STAT "\n";

while (my ($lib, $cls) = each %libClass){
    print STAT "Library:\t$lib\n";
    print STAT "no. valid links:\t$validLib{$lib}\n";
    print STAT "no. incorrect len. links:\t$lenLib{$lib}\n";
    print STAT "no. incorrect ori. links:\t$oriLib{$lib}\n";
    print STAT "no. unchecked links:\t$unusedLib{$lib}\n";
    print STAT "\n";
}

close(STAT);

exit(0);


##################
# get N50 value for an array.  Assumes array is sorted in decreasing
# order
sub N50
{
    my $arr = shift;
    my $size = shift;
    $size /= 2;
    my $tem = 0;

    for (my $i = 0; $i <= $#$arr; $i++){
	$tem += $$arr[$i];
	if ($tem > $size){
	    return $$arr[$i];
	}
    }
}

# prints details about an edge
sub printDetail
{
    my $file = shift;
    my $edge = shift;

    my ($ctgA, $ctgB) = split(' ', $edge);

    if ($contigStart{$ctgA} > $contigStart{$ctgB}){
	my $tmp = $ctgA;
	$ctgA = $ctgB;
	$ctgB = $tmp;
    } # orient from leftmost to rightmost

    my @valids = split(' ', $validLs{$edge});
    my @lens = split(' ', $invalidLens{$edge});
    my @oris = split(' ', $invalidOris{$edge});

    my $uu;
    if ($doingUnused) {
	$uu = "U";
    } else {
	$uu = "";
    }

    print $file "\n$uu           $ctgA ($contigStart{$ctgA}, $contigEnd{$ctgA})";

    if ($contigOri{$ctgA} eq "BE"){
	print $file " ====>";
    } else {
	print $file " <====";
    }

    print $file " v:", $#valids + 1, " l:", $#lens + 1, " o:", $#oris + 1, " ";
    
    if ($contigOri{$ctgB} eq "BE"){
	print $file "====> ";
    } else {
	print $file "<==== ";
    }

    print $file "$ctgB ($contigStart{$ctgB}, $contigEnd{$ctgB})";

    print $file "\n";

    my %libs = ();
    print $file "Valid:\n";
    for (my $i = 0; $i <= $#valids; $i++){
	push(@{$libs{$insertLib{$valids[$i]}}}, $valids[$i]);
    }
    while (my ($lib, $inserts) = each %libs){
	print $file "  library $lib:\n";
	for (my $i = 0; $i <= $#$inserts; $i++){
	    printDetailInsert($file, $$inserts[$i], $ctgA, $ctgB);
	}
    }

    %libs = ();
    if ($#lens >= 0){
	print $file "Invalid length:\n";
	for (my $i = 0; $i <= $#lens; $i++){
	    push(@{$libs{$insertLib{$lens[$i]}}}, $lens[$i]);
	}
	while (my ($lib, $inserts) = each %libs){
	    print $file "  library $lib:\n";
	    for (my $i = 0; $i <= $#$inserts; $i++){
		printDetailInsert($file, $$inserts[$i], $ctgA, $ctgB);
	    }
	}
    }
    %libs = ();
    if ($#oris >= 0){
	print $file "Invalid orientation:\n";
	for (my $i = 0; $i <= $#oris; $i++){
	    push(@{$libs{$insertLib{$oris[$i]}}}, $oris[$i]);
	}
	while (my ($lib, $inserts) = each %libs){
	    print $file "  library $lib:\n";
	    for (my $i = 0; $i <= $#$inserts; $i++){
		printDetailInsert($file, $$inserts[$i], $ctgA, $ctgB);
	    }
	}
    }
    
    print $file "\n";
    
} # printDetail

sub printDetailInsert
{
    my $file = shift;
    my $insert = shift;
    my $ctgA = shift;
    my $ctgB = shift;

    if (exists $inserts{$insert}){

	my ($seqA, $seqB) = split(' ', $inserts{$insert});
	if (! defined $seqA || ! defined $seqB){
	    $base->logError("Can't find sequences for insert $insert", 1);
	    return;
	}
	
	if ($seqCtg{$seqA} eq $ctgB){
	    my $tmp = $seqB;
	    $seqB = $seqA;
	    $seqA = $tmp;
	}

	my $len;  # here we try to compute clone sizes
	my $coordA; # start of seqA in global reference system
	my $coordB; # same for seqB
	
	if ($seqOri{$seqA} eq "BE"){
	    $coordA = $seqLend{$seqA};
	} else {
	    $coordA = $seqRend{$seqA};
	}
	if ($contigOri{$ctgA} eq "BE"){
	    $coordA += $contigStart{$ctgA};
	} else {
	    $coordA = $contigEnd{$ctgA} - $coordA;
	}

	if ($seqOri{$seqB} eq "BE"){
	    $coordB = $seqLend{$seqB};
	} else {
	    $coordB = $seqRend{$seqB};
	}
	if ($contigOri{$ctgB} eq "BE"){
	    $coordB += $contigStart{$ctgB};
	} else {
	    $coordB = $contigEnd{$ctgB} - $coordB;
	}
	
	$len = $coordB - $coordA;
	
	print $file "    ", sprintf("%7s %7d %7d", $seqName{$seqA}, $seqLend{$seqA}, $seqRend{$seqA});
	printSeqArrow($file, $seqA, $ctgA);
	print $file sprintf(" ... %5d ... ", $len);
	printSeqArrow($file, $seqB, $ctgB);
	print $file sprintf("%7d %7d %7s\n", $seqLend{$seqB}, $seqRend{$seqB}, $seqName{$seqB});
    } else { # MUMmer link
	print $file "    $insert:\n";
	my @ctgs = split(' ', $linkCtg{$insert});
	print $file $conEv{$ctgs[0]}, "\n";
	print $file $conEv{$ctgs[1]}, "\n";
    }
}

sub printSeqArrow
{
    my $file = shift;
    my $seq = shift;
    my $ctg = shift;

    if ($contigOri{$ctg} eq "BE"){
	if ($seqOri{$seq} eq "BE"){
	    print $file " ---> ";
	} else {
	    print $file " <--- ";
	}
    } else {
	if ($seqOri{$seq} eq "BE"){
	    print $file " <--- ";
	} else {
	    print $file " ---> ";
	}
    }
}

# here we'll generate a string with information about a link
sub linkStr
{
    my $lnk = shift;

    return "link $lnk " . join(' ', ctgPair($lnk));
}

# returns the contig pair linked by the specified link
sub ctgPair
{
    my $lnk = shift;
    my $ctgA;
    my $ctgB;
    my $scaffA;
    my $scaffB;

    if (exists $inserts{$lnk}){
	my @seqs = split(' ', $inserts{$lnk});
	if ($#seqs != 1){
	    $base->logError("Link $lnk has less than 2 seqs", 1);
	} else {
	    my $seqA = $seqs[0];
	    my $seqB = $seqs[1];
	    
	    $ctgA = $seqCtg{$seqA};
	    $ctgB = $seqCtg{$seqB};
	}
    } else { # ! exists $inserts... link is MUMmer
	my @ctgs = split(' ', $linkCtg{$lnk});
	if ($#ctgs != 1){
	    $base->logError("Link $lnk has less than 2 contigs", 1);
	    return (undef, undef, undef, undef);
	}
	$ctgA = $ctgs[0];
	$ctgB = $ctgs[1];
    }

    $scaffA = $contigScaff{$ctgA};
    $scaffB = $contigScaff{$ctgB};
    
    return ($ctgA, $scaffA, $ctgB, $scaffB);
}

# do contigs overlap?
sub overlap
{
    my $ctgA = shift;
    my $ctgB = shift;
    
    if (($contigStart{$ctgA} <= $contigEnd{$ctgB} && 
	 $contigStart{$ctgA} >= $contigStart{$ctgB}) ||
	($contigEnd{$ctgA} <= $contigEnd{$ctgB} &&
	 $contigEnd{$ctgA} >= $contigStart{$ctgB}) ||
	($contigStart{$ctgB} <= $contigEnd{$ctgA} && 
	 $contigStart{$ctgB} >= $contigStart{$ctgA}) ||
	($contigEnd{$ctgB} <= $contigEnd{$ctgA} &&
	 $contigEnd{$ctgB} >= $contigStart{$ctgA})){
	return 1;
    } else {
	return 0;
    }
}

## XML functions ##
sub StartDocument
{
    print STDERR "starting\n";
}

sub StartTag
{
    my $tag = $_[1];
   
    if ($tag eq "EVIDENCE"){
	$inEvidence = 1;
    } elsif ($tag eq "LIBRARY") {
	$libId = $_{ID};
	$libraries{$libId} = "$_{MIN} $_{MAX}";
	$inLib = 1;
    } elsif ($tag eq "INSERT"){
	if (! $inLib){
	    $base->bail("Cannot have insert outside of library");
	}
	$inInsert = 1;
	$insId = $_{ID};
	$inserts{$insId} = "";
	$insertLib{$insId} = $libId;
    } elsif ($tag eq "CONTIG"){
	$conId = $_{ID};
	if ($inEvidence){
	    if ($inLink){
		$inContig = 1;
		$linkCtg{$id} .= "$conId ";
	    } else {
		$contigs{$conId} = $_{LEN};
	    }
	} else {
	    $contigX{$conId} = $_{X};
	    $contigStart{$conId} = ($_{ORI} eq "BE") ? 
		$_{X} : ($_{X} - $contigs{$conId});
	    $contigEnd{$conId} = ($_{ORI} eq "BE") ?
		($_{X} + $contigs{$conId}) : $_{X};
	    $contigOri{$conId} = $_{ORI};
	    $contigScaff{$conId} = $scaffId;
	    $scaffContig{$scaffId} .= "$conId ";
	    $scaffSize{$scaffId} += $contigs{$conId};
	}
    } elsif ($tag eq "SEQUENCE"){
	my $id = $_{ID};
	if ($inInsert){
	    $inserts{$insId} .= "$id ";
	    $seqName{$id} = $_{NAME};
	} else { # we are in contigs
	    $seqCtg{$id} = $conId;
	    $seqOri{$id} = $_{ORI};
	    $seqLend{$id} = $_{ASM_LEND};
	    $seqRend{$id} = $_{ASM_REND};
	}
    } elsif ($tag eq "SCAFFOLD") {
	$scaffId = $_{ID};
	push(@scaffolds, $scaffId);
    } elsif ($tag eq "LINK") {
	$id = $_{ID};
	if ($inEvidence){ #&& ($_{TYPE} eq "MUMmer" || $_{TYPE} eq "CA")){
	    if (! exists $libraries{$_{TYPE}}){
		$libraries{$_{TYPE}} = "0 0";
	    }
	    $insertLib{$id} = $_{TYPE}; # all MUMmer links belong to the MUMmer library
	    $inLink = 1;
	} else {
	    if ($inUnused) {
		push(@unseen, $id);
	    } else {
		$scaffLink{$scaffId} .= "$id ";
	    }
	    $linkValid{$id} = $_{VALID};
	}
    } elsif ($tag eq "UNUSED") {
	$inUnused = 1;
    } else {
#	print "Unknown tag: $tag\n";
    }
}

sub EndTag
{
    my $tag = $_[1];
    if ($tag eq "INSERT"){
	$inInsert = 0;
    } elsif ($tag eq "LIBRARY"){
	$inLib = 0;
    } elsif ($tag eq "UNUSED"){
	$inUnused = 0;
    } elsif ($tag eq "EVIDENCE"){
	$inEvidence = 0;
    } elsif ($tag eq "LINK"){
	if ($inEvidence){
	    $inLink = 0;
	}
    } elsif ($tag eq "CONTIG"){
	if ($inLink){
	    $inContig = 0;
	}
    }
}

sub Text
{
    if ($inContig){
	$conEv{$conId} = $_;
    }
}

sub pi
{
}

sub EndDocument
{
    print STDERR "Done\n";
}
