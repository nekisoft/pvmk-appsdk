/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
// Original Copyright notice:
// Copyright (C) 1991 Texas Instruments Incorporated.
//
// Permission is granted to any individual or institution to use, copy, modify,
// and distribute this software, provided that this complete copyright and
// permission notice is maintained, intact, in all copies and supporting
// documentation.
//
// Texas Instruments Incorporated provides this software "as is" without
// express or implied warranty.
//
// Created: MNF 06/13/89  Initial Design and Implementation
// Updated: LGO 08/09/89  Inherit from Generic
// Updated: MBN 09/07/89  Added conditional exception handling
// Updated: MBN 12/15/89  Sprinkled "const" qualifiers all over the place!
// Updated: DLS 03/22/91  New lite version
//

#ifndef cmsys_RegularExpression_hxx
#define cmsys_RegularExpression_hxx

#include <cmsys/Configure.h>
#include <cmsys/Configure.hxx>

#include <string>

namespace cmsys {

// Forward declaration
class RegularExpression;

/** \class RegularExpressionMatch
 * \brief Stores the pattern matches of a RegularExpression
 */
class cmsys_EXPORT RegularExpressionMatch
{
public:
  RegularExpressionMatch();

  bool isValid() const;
  void clear();

  std::string::size_type start(int n = 0) const;
  std::string::size_type end(int n = 0) const;
  std::string match(int n = 0) const;

  enum
  {
    NSUBEXP = 32
  };

private:
  friend class RegularExpression;
  char const* startp[NSUBEXP];
  char const* endp[NSUBEXP];
  char const* searchstring;
};

#ifdef _MSC_VER
#  pragma warning(push)
#  if _MSC_VER < 1900
#    pragma warning(disable : 4351) /* new behavior */
#  endif
#endif

/**
 * \brief Creates an invalid match object
 */
inline RegularExpressionMatch::RegularExpressionMatch()
  : startp{}
  , endp{}
  , searchstring{}
{
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

/**
 * \brief Returns true if the match pointers are valid
 */
inline bool RegularExpressionMatch::isValid() const
{
  return (this->startp[0]);
}

/**
 * \brief Resets to the (invalid) construction state.
 */
inline void RegularExpressionMatch::clear()
{
  startp[0] = nullptr;
  endp[0] = nullptr;
  searchstring = nullptr;
}

/**
 * \brief Returns the start index of nth submatch.
 *        start(0) is the start of the full match.
 */
inline std::string::size_type RegularExpressionMatch::start(int n) const
{
  if (!this->startp[n]) {
    return std::string::npos;
  }
  return static_cast<std::string::size_type>(this->startp[n] -
                                             this->searchstring);
}

/**
 * \brief Returns the end index of nth submatch.
 *        end(0) is the end of the full match.
 */
inline std::string::size_type RegularExpressionMatch::end(int n) const
{
  if (!this->endp[n]) {
    return std::string::npos;
  }
  return static_cast<std::string::size_type>(this->endp[n] -
                                             this->searchstring);
}

/**
 * \brief Returns the nth submatch as a string.
 */
inline std::string RegularExpressionMatch::match(int n) const
{
  if (!this->startp[n]) {
    return std::string();
  } else {
    return std::string(
      this->startp[n],
      static_cast<std::string::size_type>(this->endp[n] - this->startp[n]));
  }
}

/** \class RegularExpression
 * \brief Implements pattern matching with regular expressions.
 *
 * This is the header file for the regular expression class.  An object of
 * this class contains a regular expression, in a special "compiled" format.
 * This compiled format consists of several slots all kept as the objects
 * private data.  The RegularExpression class provides a convenient way to
 * represent regular expressions.  It makes it easy to search for the same
 * regular expression in many different strings without having to compile a
 * string to regular expression format more than necessary.
 *
 * This class implements pattern matching via regular expressions.
 * A regular expression allows a programmer to specify  complex
 * patterns  that  can  be searched for and matched against the
 * character string of a string object. In its simplest form, a
 * regular  expression  is  a  sequence  of  characters used to
 * search for exact character matches. However, many times  the
 * exact  sequence to be found is not known, or only a match at
 * the beginning or end of a string is desired. The RegularExpression regu-
 * lar  expression  class implements regular expression pattern
 * matching as is found and implemented in many  UNIX  commands
 * and utilities.
 *
 * Example: The perl code
 *
 *    $filename =~ m"([a-z]+)\.cc";
 *    print $1;
 *
 * Is written as follows in C++
 *
 *    RegularExpression re("([a-z]+)\\.cc");
 *    re.find(filename);
 *    cerr << re.match(1);
 *
 *
 * The regular expression class provides a convenient mechanism
 * for  specifying  and  manipulating  regular expressions. The
 * regular expression object allows specification of such  pat-
 * terns  by using the following regular expression metacharac-
 * ters:
 *
 *  ^        Matches at beginning of a line
 *
 *  $        Matches at end of a line
 *
 * .         Matches any single character
 *
 * [ ]       Matches any character(s) inside the brackets
 *
 * [^ ]      Matches any character(s) not inside the brackets
 *
 *  -        Matches any character in range on either side of a dash
 *
 *  *        Matches preceding pattern zero or more times
 *
 *  +        Matches preceding pattern one or more times
 *
 *  ?        Matches preceding pattern zero or once only
 *
 * ()        Saves a matched expression and uses it in a later match
 *
 * Note that more than one of these metacharacters can be  used
 * in  a  single  regular expression in order to create complex
 * search patterns. For example, the pattern [^ab1-9]  says  to
 * match  any  character  sequence that does not begin with the
 * characters "ab"  followed  by  numbers  in  the  series  one
 * through nine.
 *
 * There are three constructors for RegularExpression.  One just creates an
 * empty RegularExpression object.  Another creates a RegularExpression
 * object and initializes it with a regular expression that is given in the
 * form of a char*.  The third takes a reference to a RegularExpression
 * object as an argument and creates an object initialized with the
 * information from the given RegularExpression object.
 *
 * The  find  member function  finds   the  first  occurrence  of  the regular
 * expression of that object in the string given to find as an argument.  Find
 * returns a boolean, and  if true,  mutates  the private  data appropriately.
 * Find sets pointers to the beginning and end of  the thing last  found, they
 * are pointers into the actual string  that was searched.   The start and end
 * member functions return indices  into the searched string that  correspond
 * to the beginning   and  end pointers  respectively.   The    compile member
 * function takes a char* and puts the  compiled version of the char* argument
 * into the object's private data fields.  The == and  != operators only check
 * the  to see  if   the compiled  regular  expression   is the same, and  the
 * deep_equal functions also checks  to see if the  start and end pointers are
 * the same.  The is_valid  function returns false if  program is set to
 * nullptr, (i.e. there is no valid compiled  expression).  The  set_invalid
 * function sets the program to nullptr (Warning:  this deletes the compiled
 * expression). The following examples may help clarify regular expression
 * usage:
 *
 *   *  The regular expression  "^hello" matches  a "hello"  only at  the
 *      beginning of a  line.  It would match "hello  there" but not "hi,
 *      hello there".
 *
 *   *  The regular expression "long$" matches a  "long"  only at the end
 *      of a line. It would match "so long\0", but not "long ago".
 *
 *   *  The regular expression "t..t..g"  will match anything that  has a
 *      "t" then any two characters, another "t", any  two characters and
 *      then a "g".   It will match  "testing", or "test again" but would
 *      not match "toasting"
 *
 *   *  The regular  expression "[1-9ab]" matches any  number one through
 *      nine, and the characters  "a" and  "b".  It would match "hello 1"
 *      or "begin", but would not match "no-match".
 *
 *   *  The  regular expression "[^1-9ab]"  matches any character that is
 *      not a number one  through nine, or  an "a" or "b".   It would NOT
 *      match "hello 1" or "begin", but would match "no-match".
 *
 *   *  The regular expression "br* " matches  something that begins with
 *      a "b", is followed by zero or more "r"s, and ends in a space.  It
 *      would match "brrrrr ", and "b ", but would not match "brrh ".
 *
 *   *  The regular expression "br+ " matches something  that begins with
 *      a "b", is followed by one or more "r"s, and ends in  a space.  It
 *      would match "brrrrr ",  and  "br ", but would not  match "b  " or
 *      "brrh ".
 *
 *   *  The regular expression "br? " matches  something that begins with
 *      a "b", is followed by zero or one "r"s, and ends in  a space.  It
 *      would  match  "br ", and "b  ", but would not match  "brrrr "  or
 *      "brrh ".
 *
 *   *  The regular expression "(..p)b" matches  something ending with pb
 *      and beginning with whatever the two characters before the first p
 *      encountered in the line were.  It would find "repb" in "rep drepa
 *      qrepb".  The regular expression "(..p)a"  would find "repa qrepb"
 *      in "rep drepa qrepb"
 *
 *   *  The regular expression "d(..p)" matches something ending  with p,
 *      beginning with d, and having  two characters  in between that are
 *      the same as the two characters before  the first p encountered in
 *      the line.  It would match "drepa qrepb" in "rep drepa qrepb".
 *
 * All methods of RegularExpression can be called simultaneously from
 * different threads but only if each invocation uses an own instance of
 * RegularExpression.
 */
class cmsys_EXPORT RegularExpression
{
public:
  enum Options : unsigned
  {
    // Match ^ at offset instead of the input start.
    BOL_AT_OFFSET = 1,
    // If an empty match is found at offset, continue searching.
    NONEMPTY_AT_OFFSET = 2,
  };

  /**
   * Instantiate RegularExpression with program=nullptr.
   */
  inline RegularExpression();

  /**
   * Instantiate RegularExpression with compiled char*.
   */
  inline RegularExpression(char const*);

  /**
   * Instantiate RegularExpression as a copy of another regular expression.
   */
  RegularExpression(RegularExpression const&);

  /**
   * Instantiate RegularExpression with compiled string.
   */
  inline RegularExpression(std::string const&);

  /**
   * Destructor.
   */
  inline ~RegularExpression();

  /**
   * Compile a regular expression into internal code
   * for later pattern matching.
   */
  bool compile(char const*);

  /**
   * Compile a regular expression into internal code
   * for later pattern matching.
   */
  inline bool compile(std::string const&);

  /**
   * Matches the regular expression to the given string.
   * Returns true if found, and sets start and end indexes
   * in the RegularExpressionMatch instance accordingly.
   *
   * This method is thread safe when called with different
   * RegularExpressionMatch instances.
   */
  bool find(char const*, RegularExpressionMatch&,
            std::string::size_type offset = 0, unsigned options = 0) const;

  /**
   * Matches the regular expression to the given string.
   * Returns true if found, and sets start and end indexes accordingly.
   */
  inline bool find(char const*, std::string::size_type offset = 0,
                   unsigned options = 0);

  /**
   * Matches the regular expression to the given std string.
   * Returns true if found, and sets start and end indexes accordingly.
   */
  inline bool find(std::string const&, std::string::size_type offset = 0,
                   unsigned options = 0);

  /**
   * Match indices
   */
  inline RegularExpressionMatch const& regMatch() const;
  inline std::string::size_type start(int n = 0) const;
  inline std::string::size_type end(int n = 0) const;

  /**
   * Match strings
   */
  inline std::string match(int n = 0) const;

  /**
   * Copy the given regular expression.
   */
  RegularExpression& operator=(RegularExpression const& rxp);

  /**
   * Returns true if two regular expressions have the same
   * compiled program for pattern matching.
   */
  bool operator==(RegularExpression const&) const;

  /**
   * Returns true if two regular expressions have different
   * compiled program for pattern matching.
   */
  inline bool operator!=(RegularExpression const&) const;

  /**
   * Returns true if have the same compiled regular expressions
   * and the same start and end pointers.
   */
  bool deep_equal(RegularExpression const&) const;

  /**
   * True if the compiled regexp is valid.
   */
  inline bool is_valid() const;

  /**
   * Marks the regular expression as invalid.
   */
  inline void set_invalid();

  /**
   * The number of capture groups.
   */
  inline int num_groups();

private:
  RegularExpressionMatch regmatch;
  char regstart;                  // Internal use only
  char reganch;                   // Internal use only
  char const* regmust;            // Internal use only
  std::string::size_type regmlen; // Internal use only
  char* program;
  int progsize;
  int regnpar;
};

/**
 * Create an empty regular expression.
 */
inline RegularExpression::RegularExpression()
  : regstart{}
  , reganch{}
  , regmust{}
  , program{ nullptr }
  , progsize{}
  , regnpar{}
{
}

/**
 * Creates a regular expression from string s, and
 * compiles s.
 */
inline RegularExpression::RegularExpression(char const* s)
  : regstart{}
  , reganch{}
  , regmust{}
  , program{ nullptr }
  , progsize{}
  , regnpar{}
{
  if (s) {
    this->compile(s);
  }
}

/**
 * Creates a regular expression from string s, and
 * compiles s.
 */
inline RegularExpression::RegularExpression(std::string const& s)
  : regstart{}
  , reganch{}
  , regmust{}
  , program{ nullptr }
  , progsize{}
  , regnpar{}
{
  this->compile(s);
}

/**
 * Destroys and frees space allocated for the regular expression.
 */
inline RegularExpression::~RegularExpression()
{
  // #ifndef _WIN32
  delete[] this->program;
  // #endif
}

/**
 * Compile a regular expression into internal code
 * for later pattern matching.
 */
inline bool RegularExpression::compile(std::string const& s)
{
  return this->compile(s.c_str());
}

/**
 * Matches the regular expression to the given std string.
 * Returns true if found, and sets start and end indexes accordingly.
 */
inline bool RegularExpression::find(char const* s,
                                    std::string::size_type offset,
                                    unsigned options)
{
  return this->find(s, this->regmatch, offset, options);
}

/**
 * Matches the regular expression to the given std string.
 * Returns true if found, and sets start and end indexes accordingly.
 */
inline bool RegularExpression::find(std::string const& s,
                                    std::string::size_type offset,
                                    unsigned options)
{
  return this->find(s.c_str(), this->regmatch, offset, options);
}

/**
 * Returns the internal match object
 */
inline RegularExpressionMatch const& RegularExpression::regMatch() const
{
  return this->regmatch;
}

/**
 * Return start index of nth submatch. start(0) is the start of the full match.
 */
inline std::string::size_type RegularExpression::start(int n) const
{
  return regmatch.start(n);
}

/**
 * Return end index of nth submatch. end(0) is the end of the full match.
 */
inline std::string::size_type RegularExpression::end(int n) const
{
  return regmatch.end(n);
}

/**
 * Return nth submatch as a string.
 */
inline std::string RegularExpression::match(int n) const
{
  return regmatch.match(n);
}

/**
 * Returns true if two regular expressions have different
 * compiled program for pattern matching.
 */
inline bool RegularExpression::operator!=(RegularExpression const& r) const
{
  return (!(*this == r));
}

/**
 * Returns true if a valid regular expression is compiled
 * and ready for pattern matching.
 */
inline bool RegularExpression::is_valid() const
{
  return (this->program);
}

inline void RegularExpression::set_invalid()
{
  // #ifndef _WIN32
  delete[] this->program;
  // #endif
  this->program = nullptr;
}

inline int RegularExpression::num_groups()
{
  return this->regnpar - 1;
}

} // namespace cmsys

#endif
