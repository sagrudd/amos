////////////////////////////////////////////////////////////////////////////////
//! \file
//! \author Adam M Phillippy
//! \date 06/18/2003
//!
//! \brief Header for Read_t
//!
////////////////////////////////////////////////////////////////////////////////

#ifndef __Read_AMOS_HH
#define __Read_AMOS_HH 1

#include "Sequence_AMOS.hh"




namespace AMOS {

//================================================ Read_t ======================
//! \brief A DNA sequencing read
//!
//! Derived from a Sequence_t type, this class is for the efficient storage of
//! DNA sequencing reads. Retains all the functionality of a Sequence with
//! added fields for clear ranges, mate pairs, etc.
//!
//! \todo add more clear range flexibility
//==============================================================================
class Read_t : public Sequence_t
{

public:

  enum ReadType_t
  //!< read description
    {
      NULL_READ = 0,
      OTHER,
      NORMAL,
      CONTIG,
      BAC,
      WALK,
      MAX_READS,
    };


private:

  std::string eid_m;               //!< the external string ID
  ID_t frag_m;                     //!< the ID of the parent fragment
  Range_t qclear_m;                //!< the quality score clear range
  ReadType_t type_m;               //!< the read type
  Range_t vclear_m;                //!< the vector clear range


protected:

  //--------------------------------------------------- readRecord -------------
  //! \brief Read all the class members from a biserial record
  //!
  //! Reads the fixed and variable length streams from a biserial record and
  //! initializes all the class members to the values stored within. Used in
  //! translating a biserial Bankable object, and needed to retrieve objects
  //! from a Bank. Returned size of the record will only be valid if the read
  //! was successful, i.e. fix.good( ) and var.good( ).
  //!
  //! \note This method must be able to interpret the biserial record
  //! produced by its related function writeRecord.
  //!
  //! \param fix The fixed length stream (stores all fixed length members)
  //! \param var The variable length stream (stores all var length members)
  //! \pre The get pointer of fix is at the beginning of the record
  //! \pre The get pointer of var is at the beginning of the record
  //! \return size of read record (size of fix + size of var)
  //!
  Size_t readRecord (std::istream & fix,
		     std::istream & var);


  //--------------------------------------------------- writeRecord ------------
  //! \brief Write all the class members to a biserial record
  //!
  //! Writes the fixed an variable length streams to a biserial record. Used in
  //! generating a biserial Bankable object, and needed to commit objects to a
  //! Bank. Will only write to the ready streams, but the size of the record
  //! will always be returned.
  //!
  //! \note This method must be able to produce a biserial record that can
  //! be read by its related funtion readRecord.
  //!
  //! \param fix The fixed length stream (stores all fixed length members)
  //! \param var The variable length stream (stores all var length members)
  //! \return size of written record (size of fix + size of var)
  //!
  Size_t writeRecord (std::ostream & fix,
		      std::ostream & var) const;


public:

  static const BankType_t BANKTYPE = Bankable_t::READ;
  //!< Bank type, MUST BE UNIQUE for all derived Bankable classes!


  //--------------------------------------------------- Read_t -----------------
  //! \brief Constructs an empty Read_t object
  //!
  //! Sets all members to NULL
  //!
  Read_t ( )
  {
    frag_m = NULL_ID;
    type_m = NULL_READ;
  }


  //--------------------------------------------------- Read_t -----------------
  //! \brief Copy constructor
  //!
  Read_t (const Read_t & source)
  {
    *this = source;
  }


  //--------------------------------------------------- ~Read_t ----------------
  //! \brief Destroys a Read_t object
  //!
  ~Read_t ( )
  {

  }


  //--------------------------------------------------- getBankType ------------
  //! \brief Get the unique bank type identifier
  //!
  //! \return The unique bank type identifier
  //!
  BankType_t getBankType ( ) const
  {
    return BANKTYPE;
  }


  //--------------------------------------------------- getEID -----------------
  //! \brief Get the external ID string
  //!
  //! \return The external ID string
  //!
  std::string & getEID ( )
  {
    return eid_m;
  }


  //--------------------------------------------------- getFragment ------------
  //! \brief Get the parent fragment ID
  //!
  //! \return The parent fragment ID
  //!
  ID_t getFragment ( ) const
  {
    return frag_m;
  }


  //--------------------------------------------------- getQualityClearRange ---
  //! \brief Get the quality score clear range
  //!
  //! \return The quality score clear range
  //!
  Range_t getQualityClearRange ( )
  {
    return qclear_m;
  }


  //--------------------------------------------------- getType ----------------
  //! \brief Get the type of read
  //!
  //! \return The type of read
  //!
  ReadType_t getType ( ) const
  {
    return type_m;
  }


  //--------------------------------------------------- getVectorClearRange ----
  //! \brief Get the vector sequence clear range
  //!
  //! \return The vector sequence clear range
  //!
  Range_t getVectorClearRange ( )
  {
    return vclear_m;
  }


  //--------------------------------------------------- setEID -----------------
  //! \brief Set the external ID string
  //!
  //! \param eid The new external ID string
  //! \return void
  //!
  void setEID (const std::string & eid)
  {
    eid_m = eid;
  }


  //--------------------------------------------------- setFragment ------------
  //! \brief Set the parent fragment ID
  //!
  //! \param frag The new parent fragment ID
  //! \return void
  //!
  void setFragment (ID_t frag)
  {
    frag_m = frag;
  }


  //--------------------------------------------------- setQualityClearRange ---
  //! \brief Set the quality score clear range
  //!
  //! \param qclear The new quality score clear range
  //! \return void
  //!
  void setQualityClearRange (Range_t qclear)
  {
    qclear_m = qclear;
  }


  //--------------------------------------------------- setType ----------------
  //! \brief Set the type of read
  //!
  //! \param type The new type of read
  //! \return void
  //!
  void setType (ReadType_t type)
  {
    type_m = type;
  }


  //--------------------------------------------------- setVectorClearRange ----
  //! \brief Set the vector sequence clear range
  //!
  //! \param vclear The new vector sequence clear range
  //! \return void
  //!
  void setVectorClearRange (Range_t vclear)
  {
    vclear_m = vclear;
  }

};

} // namespace AMOS

#endif // #ifndef __Read_AMOS_HH
