#ifndef Read_HH
#define Read_HH 1

/**
 * The <b>Read</b> class
 *
 *
 * <p>Copyright &copy; 2004, The Institute for Genomic Research (TIGR).
 * <br>All rights reserved.
 *
 * @author  Dan Sommer
 *
 * <pre>
 * $RCSfile$
 * $Revision$
 * $Date$
 * $Author$
 * </pre>
 */
class Read {
public:
  int id;
  int len;
  int start;
  int end;

  Read(int p_id, int p_len, int p_start = -1, int p_end = -1) 
    : id(p_id), len(p_len), start(p_start), end(p_end) {  }

};

#endif // #ifndef Read_HH
