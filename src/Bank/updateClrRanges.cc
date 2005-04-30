#include "foundation_AMOS.hh"
#include "amp.hh"
#include "fasta.hh"


using namespace AMOS;
using namespace std;



int main(int argc, char ** argv)
{
  if (argc != 3)
  {
    cerr << "Usage: updateClrRanges bankname seqname.end5.end3.txt" << endl;
    return EXIT_FAILURE;
  }

  Bank_t read_bank(Read_t::NCODE);
  string bank_name = argv[1];
  string clr_name = argv[2];

  int verbose = 0;

  vector<string> badSeqs;

  cerr << "Processing " << bank_name << " at " << Date() << endl;

  try
  {
    read_bank.open(bank_name);
    Read_t read;

    FILE * fclr = fopen(clr_name.c_str(), "r");

    if (!fclr)
    {
      cerr << "Can't open clrrangefile: " << clr_name << endl;
      return EXIT_FAILURE;
    }

    int MAX_SEQNAME=100;
    char seqname[MAX_SEQNAME];
    int count = 0;
    int a,b;

    while(fscanf(fclr, "%s %d %d", seqname, &a, &b) > 0)
    {
      cerr << seqname << endl;
      if (read_bank.existsEID(seqname))
      {
        if (verbose) {  cerr << "."; }
        read_bank.fetch(seqname, read);

        Range_t range(a,b);
        if (range.isReverse()) { range.swap(); }

        range.begin--;

        read.setClearRange(range);
        read_bank.replace(read.getIID(), read);
        count++;
      }
      
      if (verbose) { cerr << endl; }
    }

    cerr << "Loaded " << count << " records." << endl;

    cerr << "Cleaning Bank" << endl;
    read_bank.clean();
  }
  catch (Exception_t & e)
  {
    cerr << "ERROR: -- Fatal AMOS Exception --\n" << e;
    return EXIT_FAILURE;
  }

  cerr << "End: " << Date() << endl;

  return EXIT_SUCCESS;
}
