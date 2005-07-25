#include "foundation_AMOS.hh"
#include "amp.hh"
#include "fasta.hh"
#include "AMOS_Foundation.hh"

using namespace AMOS;
using namespace std;

int m_alignStart(0);
string m_queryGaps("");
string m_referenceGaps("");
int m_promoteAlignmentGaps(0);
int m_reverseQuery(0);
int m_ungappedCoordinates(0);
int m_gapCoordinateOffset(0);
int m_referenceConsensus(0);


int main (int argc, char ** argv)
{

  int retval = 0;
  AMOS_Foundation * tf = NULL;

  try
  {
    string version =  "Version 1.0";
    string dependencies = "";
    string helptext = 
"Zip together a query slice file onto a reference slice file. The beginning\n"
"of the alignment is specified with -O, and gaps can be inserted into both\n"
"slice files using -r and -q.\n"
"\n"
"   Usage: zipcontigs [options] bnk qryeid refeid joineid\n"
"\n"
"   query.slice     Slice file of query contig (typically smaller than reference)\n"
"   reference.slice Slice file of reference contig\n"
"\n"
"   zipSlice Options\n"
"   ----------------\n"
"   -R Query should be reversed before merging\n"
"   -1 Coordinates of gaps are 1-based\n"
"   -U Coordinates of gaps to insert are ungapped\n"
"   -p Automatically promote preexisting gaps\n"
"   --refcons Use the consensus of the reference for merged slices\n"
"             (Uses the query consensus by default)\n"
"\n"
"   -q <csl> Comma separated list of positions to insert gaps in the query\n"
"   -r <csl> Comma separated list of positions to insert gaps in the reference\n"
"\n"
"   -O <offset> Specify alignment offset from query to reference\n"
"               (negative means reference left flanks query (always 0-based))\n";

    // Instantiate a new TIGR_Foundation object
    tf = new AMOS_Foundation (version, helptext, dependencies, argc, argv);
    tf->disableOptionHelp();
    tf->getOptions()->addOptionResult("O|alignmentoffset=i", &m_alignStart,
                                      "Specify alignment offset from query to reference\n           negative means reference left flanks query (always 0-based)");

    tf->getOptions()->addOptionResult("q|querygaps=s", &m_queryGaps,
                                      "Specify gaps in query slices");

    tf->getOptions()->addOptionResult("r|referencegaps=s", &m_referenceGaps,
                                      "Specify gaps in reference slices");

    tf->getOptions()->addOptionResult("p|promoteAlignmentGaps!", &m_promoteAlignmentGaps,
                                      "Toggle if alignment gaps should automatically be promoted");

    tf->getOptions()->addOptionResult("R|reversequery!", &m_reverseQuery,
                                      "Query should be reversed before merging");

    tf->getOptions()->addOptionResult("U|ungapped!", &m_ungappedCoordinates,
                                      "Coordinates are all ungapped");

    tf->getOptions()->addOptionResult("1", &m_gapCoordinateOffset,
                                      "Coordinates of gaps are 1-based");

    tf->getOptions()->addOptionResult("refcons", &m_referenceConsensus,
                                      "Use the consensus of the reference for the merged slices");

    tf->handleStandardOptions();

    list<string> argvv = tf->getOptions()->getAllOtherData();

    if (argvv.size() != 4)
    {
      cerr << "Usage: zipcontigs [options] bankname refeid qryeid joineid" << endl;
      return EXIT_FAILURE;
    }

    string bankname = argvv.front(); argvv.pop_front();
    string refeid = argvv.front(); argvv.pop_front();
    string qryeid = argvv.front(); argvv.pop_front();
    string jeid   = argvv.front(); argvv.pop_front();

    Bank_t contig_bank(Contig_t::NCODE);

    cerr << "Processing " << bankname << " at " << Date() << endl;

    contig_bank.open(bankname, B_READ|B_WRITE);
    ID_t jiid     = contig_bank.getMaxIID()+1;

    Contig_t rcontig, qcontig;

    contig_bank.fetch(refeid, rcontig);
    contig_bank.fetch(qryeid, qcontig);

    cerr << "Ref: " << refeid << " len: " << rcontig.getSeqString().length() << endl;
    cerr << "Qry: " << qryeid << " len: " << qcontig.getSeqString().length() << endl;

    cerr << " m_alignStart(0)=" <<           m_alignStart << endl;
    cerr << " m_queryGaps(\"\")=" <<         m_queryGaps<< endl;
    cerr << " m_referenceGaps(\"\")=" <<     m_referenceGaps<< endl;
    cerr << " m_promoteAlignmentGaps(0)=" << m_promoteAlignmentGaps<< endl;
    cerr << " m_reverseQuery(0)=" <<         m_reverseQuery<< endl;
    cerr << " m_ungappedCoordinates(0)=" <<  m_ungappedCoordinates<< endl;
    cerr << " m_gapCoordinateOffset(0)=" <<  m_gapCoordinateOffset<< endl;
    cerr << " m_referenceConsensus(0)=" <<   m_referenceConsensus<< endl;

    int shift = 0;

    string rcons(rcontig.getSeqString());
    string qcons(qcontig.getSeqString());

    Pos_t rgindex = 0;
    Pos_t qgindex = 0;

    vector <Pos_t> rgaps;
    vector <Pos_t> qgaps;

    int rlen = rcons.length();
    int qlen = qcons.length();

    while (rgindex < rlen && qgindex < qlen) 
    {
      if (rcons[rgindex] == '-' && qcons[qgindex] == '-')
      {
        // Let gaps zip together
        rgindex++;
        qgindex++;
        cerr << "-";
      }
      else if (rcons[rgindex] == '-')
      {
        qgaps.push_back(qgindex);
        rgindex++;
        cerr << "r";
      }
      else if (qcons[qgindex] == '-')
      {
        rgaps.push_back(rgindex);
        qgindex++;
        cerr << "q";
      }
      else
      {
        rgindex++;
        qgindex++;
        cerr << ".";
      }
    }


    // Apply Gaps

    cerr << cerr << "Applying " << rgaps.size() << " reference gaps" << endl;
    vector <Pos_t>::reverse_iterator gi;
    for (gi = rgaps.rbegin(); gi != rgaps.rend(); gi++)
    {
      rcontig.insertGapColumn(*gi);
    }

    cerr << "Applying " << qgaps.size() << " query gaps" << endl;
    for (gi = qgaps.rbegin(); gi != qgaps.rend(); gi++)
    {
      qcontig.insertGapColumn(*gi);
    }

    // Merge Tilings into rcontig
    vector<Tile_t> & rtiling = rcontig.getReadTiling();
    vector<Tile_t> & qtiling = qcontig.getReadTiling();
    vector<Tile_t>::iterator ti;

    cerr << "Merging " << qtiling.size() << " query reads" << endl;

    for (ti =  qtiling.begin(); ti != qtiling.end(); ti++)
    {
      ti->offset += shift;
      rtiling.push_back(*ti);
    }

    // save to bank, rcontig gets new name
    rcontig.setIID(jiid);
    rcontig.setEID(jeid);
    cerr << "Appending iid: " << jiid << " eid: " << jeid << endl;
    contig_bank.append(rcontig);
    contig_bank.close();
  }
  catch (Exception_t & e)
  {
    cerr << "ERROR: -- Fatal AMOS Exception --\n" << e;
    retval = 1;
  }
  catch (const ExitProgramNormally & e)
  {
    retval = 0;
  }
  catch (const amosException & e)
  {
    cerr << e << endl;
    retval = 100;
  }

  try
  {
    if (tf) delete tf;
  }
  catch (const amosException & e)
  {
    cerr << "amosException while deleting tf: " << e << endl;
    retval = 105;
  }

  cerr << "End: " << Date() << endl;
  return retval;
}
