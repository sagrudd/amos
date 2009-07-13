////////////////////////////////////////////////////////////////////////////////
//! \file
//!
//! Reports read overlaps inferred from CTG (contig) multi alignments.
//!
////////////////////////////////////////////////////////////////////////////////

// Note: Some olaps can be really horrible because of unnecssary
// gaps. Think about refining the alignments.

#include "foundation_AMOS.hh"
#include <getopt.h>

using namespace AMOS;
using namespace std;

#define PROGNAME "olapsFromContig"

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )

//=============================================================== Globals ======
string  OPT_BankName;                        // bank name parameter
int     OPT_MinOlapLen = 20;                 // minimum overlap len
float   OPT_MaxOlapErr = 0.05;               // maximum overlap err


//----------------------------------------------------- ParseArgs --------------
//! \brief Sets the global OPT_% values from the command line arguments
//!
//! \return void
//!
void ParseArgs (int argc, char ** argv);


//----------------------------------------------------- PrintHelp --------------
//! \brief Prints help information to cerr
//!
//! \param s The program name, i.e. argv[0]
//! \return void
//!
void PrintHelp (const char * s);


//----------------------------------------------------- PrintUsage -------------
//! \brief Prints usage information to cerr
//!
//! \param s The program name, i.e. argv[0]
//! \return void
//!
void PrintUsage (const char * s);


struct TilingOrderCmp
{
  bool operator() (const Tile_t & a, const Tile_t & b)
  { return ( a.offset < b.offset ); }
};


//========================================================= Function Defs ======
int main (int argc, char ** argv)
{
  int exitcode = EXIT_SUCCESS;

  //-- Parse the command line arguments
  ParseArgs (argc, argv);

  //-- BEGIN: MAIN EXCEPTION CATCH
  try {
  
    Bank_t readBank(Read_t::NCODE);
    readBank.open(OPT_BankName, B_SPY);
    Read_t readA, readB;

    BankStream_t contigBank(Contig_t::NCODE);
    contigBank.open(OPT_BankName, B_SPY);
    Contig_t contig;

    int nContigs = 0;
    long long loopa = 0;
    long long loopb = 0;
    long long loopc = 0;
    while ( contigBank >> contig )
      {
        //cerr << "\rContig " << ++nContigs;

        vector<Tile_t> & tiling = contig.getReadTiling();
        sort(tiling.begin(), tiling.end(), TilingOrderCmp());

        vector<Tile_t>::iterator ati;
        vector<Tile_t>::iterator bti;
        for ( ati = tiling.begin(); ati != tiling.end(); ++ati )
          {
            for ( bti = ati+1; bti != tiling.end(); ++bti )
              {
                if ( bti->offset > ati->offset + ati->getGappedLength() - 1 )
                  break;

                //-- Potential overlap between read A and B
                Size_t aOlapHang = bti->offset - ati->offset;
                Size_t bOlapHang =
                  (bti->offset + bti->getGappedLength()) -
                  (ati->offset + ati->getGappedLength());
                Pos_t lOlapPos = bti->offset;
                Pos_t rOlapPos =
                  MIN(ati->offset + ati->getGappedLength() - 1,
                      bti->offset + bti->getGappedLength() - 1);
                Size_t aOlapLen = rOlapPos - lOlapPos + 1;
                Size_t bOlapLen = rOlapPos - lOlapPos + 1;
                vector<Pos_t> aGaps;
                vector<Pos_t> bGaps;

                sort(ati->gaps.begin(), ati->gaps.end());
                sort(bti->gaps.begin(), bti->gaps.end());

                //-- Adjust hangs for alignment gaps
                vector<Pos_t>::const_iterator agi = ati->gaps.begin();
                vector<Pos_t>::const_iterator bgi = bti->gaps.begin();
                while ( agi != ati->gaps.end() || bgi != bti->gaps.end() )
                  {
                    Pos_t gapPos;
                    Pos_t aGapPos = MAX_POS;
                    Pos_t bGapPos = MAX_POS;

                    //-- gapped contig gap pos calculation
                    // gapped offset + ungapped gap pos + #previous gaps + 1
                    if ( agi != ati->gaps.end() )
                      {
                        aGapPos =
                          ati->offset + *agi + (agi - ati->gaps.begin()) + 1;
                      }
                    if ( bgi != bti->gaps.end() )
                      {
                        bGapPos =
                          bti->offset + *bgi + (bgi - bti->gaps.begin()) + 1;
                      }
                    if ( aGapPos == bGapPos )
                      {
                        // remove double gap in overlap
                        ++agi;
                        ++bgi;
                        aOlapLen--;
                        bOlapLen--;
                      }
                    else
                      {
                        // check left-most gap
                        if ( aGapPos < bGapPos )
                          { gapPos = aGapPos; ++agi; }
                        else
                          { gapPos = bGapPos; ++bgi; }

                        if ( gapPos < lOlapPos )
                          {
                            // shrink a hang
                            if ( aOlapHang > 0 ) aOlapHang--;
                            else aOlapHang++;
                          }
                        else if ( gapPos > rOlapPos )
                          {
                            // shrink b hang
                            if ( bOlapHang > 0 ) bOlapHang--;
                            else bOlapHang++;
                          }
                        else
                          {
                            // overlap gaps
                            if ( aGapPos < bGapPos )
                              {
                                aOlapLen--; // a gap
                                aGaps.push_back(*(agi-1));
                              }
                            else
                              {
                                bOlapLen--; // b gap
                                bGaps.push_back(*(bgi-1));
                              }
                          }
                      }
                  }

                // fix hangs if a is reversed
                if ( ati->range.isReverse() )
                  {
                    Size_t oaOlapHang = aOlapHang;
                    Size_t obOlapHang = bOlapHang;
                    aOlapHang = -obOlapHang;
                    bOlapHang = -oaOlapHang;
                  }

                // skip if olap is too short
                if ( aOlapLen < OPT_MinOlapLen && bOlapLen < OPT_MinOlapLen )
                  continue;

                char olapType =
                  ati->range.isReverse() == bti->range.isReverse() ? 'N' : 'I';


                //-- Get reads and check error rate of olap
                readBank.fetch(ati->source, readA);
                readBank.fetch(bti->source, readB);

                Size_t aLen = readA.getClearRange().getLength();
                Size_t bLen = readB.getClearRange().getLength();
                Size_t alignLen = aLen + aGaps.size();
                if ( aOlapHang < 0 ) alignLen -= aOlapHang;
                if ( bOlapHang > 0 ) alignLen += bOlapHang;

                char alignA[alignLen+1];
                char alignB[alignLen+1];
                for ( int i = 0; i < alignLen; i++ )
                  {
                    alignA[i] = '.';
                    alignB[i] = '.';
                  }
                alignA[alignLen] = '\0';
                alignB[alignLen] = '\0';

                int aOff = 0;
                int bOff = 0;
                if ( aOlapHang < 0 )
                  aOff -= aOlapHang;
                else
                  bOff += aOlapHang;

                string A = readA.getSeqString(readA.getClearRange());
                string B = readB.getSeqString(readB.getClearRange());

                // too lazy to do the math...
                if ( ati->range.isReverse() )
                  ReverseComplement(A);
                if ( bti->range.isReverse() )
                  ReverseComplement(B);

                for ( int i = 0; i < aGaps.size(); ++i )
                  A.insert(aGaps[i]+i, 1, '-');
                for ( int i = 0; i < bGaps.size(); ++i )
                  B.insert(bGaps[i]+i, 1, '-');

                if ( ati->range.isReverse() )
                  ReverseComplement(A);
                if ( bti->range.isReverse() )
                  ReverseComplement(B);

                if ( olapType == 'I' )
                  ReverseComplement(B);

                for ( int i = 0; i < A.size(); ++i )
                  alignA[aOff+i] = A[i];
                for ( int i = 0; i < B.size(); ++i )
                  alignB[bOff+i] = B[i];

                int olapErrs = 0;
                for ( int i = 0; i < alignLen; ++i )
                  if ( alignA[i] == '.' || alignB[i] == '.' )
                    continue;
                  else if ( alignA[i] != alignB[i] )
                    olapErrs++;

                float aOlapErr = float(olapErrs) / float(aOlapLen);
                float bOlapErr = float(olapErrs) / float(bOlapLen);

                // skip if olap too noisy
                if ( aOlapErr > OPT_MaxOlapErr || bOlapErr > OPT_MaxOlapErr )
                  continue;

                // report good overlap
                printf("%d\t%d\t%c\t%d\t%d\t%.2f\t%.2f\n",
                       ati->source, bti->source, olapType,
                       aOlapHang, bOlapHang,
                       aOlapErr*100.0,  bOlapErr*100.0);

                if ( 0 )
                  {
                    cout << "A: ";
                    for ( int i = 0; i < aGaps.size(); ++i )
                      cout << aGaps[i] << " ";
                    cout << "B: ";
                    for ( int i = 0; i < bGaps.size(); ++i )
                      cout << bGaps[i] << " ";
                    cout << endl;

                    cout << alignA << endl;
                    cout << alignB << endl;
                   
                    for ( int i = 1; i <= alignLen; ++i )
                      {
                        if ( i % 10 == 0 )
                          putchar('|');
                        else
                          putchar(' ');
                      }
                    putchar('\n');
                  }
              }
          }
      }
    cerr << " ... done.\n";

    contigBank.close();
  }
  catch (const Exception_t & e) {
    cerr << "FATAL: " << e . what( ) << endl
         << "  there has been a fatal error, abort" << endl;
    exitcode = EXIT_FAILURE;
  }
  //-- END: MAIN EXCEPTION CATCH
  
  return exitcode;
}



//------------------------------------------------------------- ParseArgs ------
void ParseArgs (int argc, char ** argv)
{
  int ch, errflg = 0;
  optarg = NULL;

  while ( !errflg && ((ch = getopt(argc, argv, "he:o:")) != EOF) )
    switch (ch)
      {
      case 'h':
        PrintHelp(PROGNAME);
        exit(EXIT_SUCCESS);
      break;

      case 'e':
        OPT_MaxOlapErr = atof(optarg);
        break;

      case 'o':
        OPT_MinOlapLen = atoi(optarg);
        break;

      default:
        errflg ++;
      }

  if ( optind+1 == argc )
    OPT_BankName = argv[optind++];
  else
    errflg ++;

  if ( OPT_MaxOlapErr < 0 || OPT_MaxOlapErr > 1 )
    {
      cerr << "Error: Maximum overlap error must be 0 < err < 1\n";
      errflg++;
    }

  if ( OPT_MinOlapLen <= 0 )
    {
      cerr << "Error: Minimum overlap length must be > 0\n";
      errflg++;
    }

  if ( errflg > 0 )
    {
      PrintUsage(PROGNAME);
      cerr << "Try '" << PROGNAME << " -h' for more information.\n";
      exit(EXIT_FAILURE);
    }
}


//------------------------------------------------------------- PrintHelp ------
void PrintHelp (const char * s)
{
  PrintUsage(s);

  cerr
    << "-e float      Set maximum overlap error, default "
    << OPT_MaxOlapErr << endl
    << "-o int        Set minimum overlap length, default "
    << OPT_MinOlapLen << endl
    << endl;
  cerr
    << "  Converts bank CTG messages to overlaps.\n"
    << endl;

  return;
}


//------------------------------------------------------------ PrintUsage ------
void PrintUsage (const char * s)
{
  cerr << "\nUSAGE: " << s << " [OPTION]... BANK\n";
}
