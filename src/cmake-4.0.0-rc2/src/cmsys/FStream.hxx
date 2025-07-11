/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_FStream_hxx
#define cmsys_FStream_hxx

#include <cmsys/Configure.hxx>

#include <cmsys/Encoding.hxx>

#include <fstream>
#if defined(_WIN32)
#  if !defined(_MSC_VER) && cmsys_CXX_HAS_EXT_STDIO_FILEBUF_H
#    include <ext/stdio_filebuf.h>
#  endif
#endif

namespace cmsys {
#if defined(_WIN32) &&                                                        \
  (defined(_MSC_VER) || cmsys_CXX_HAS_EXT_STDIO_FILEBUF_H)
#  if defined(_NOEXCEPT)
#    define cmsys_FStream_NOEXCEPT _NOEXCEPT
#  else
#    define cmsys_FStream_NOEXCEPT
#  endif

#  if defined(_MSC_VER)

template <typename CharType, typename Traits>
class basic_filebuf : public std::basic_filebuf<CharType, Traits>
{
#    if _MSC_VER >= 1400
public:
  typedef std::basic_filebuf<CharType, Traits> my_base_type;
  basic_filebuf* open(char const* s, std::ios_base::openmode mode)
  {
    std::wstring const wstr = Encoding::ToWindowsExtendedPath(s);
    return static_cast<basic_filebuf*>(my_base_type::open(wstr.c_str(), mode));
  }
#    endif
};

#  else

inline std::wstring getcmode(std::ios_base::openmode const mode)
{
  std::wstring cmode;
  bool plus = false;
  if (mode & std::ios_base::app) {
    cmode += L"a";
    plus = mode & std::ios_base::in ? true : false;
  } else if (mode & std::ios_base::trunc ||
             (mode & std::ios_base::out && (mode & std::ios_base::in) == 0)) {
    cmode += L"w";
    plus = mode & std::ios_base::in ? true : false;
  } else {
    cmode += L"r";
    plus = mode & std::ios_base::out ? true : false;
  }
  if (plus) {
    cmode += L"+";
  }
  if (mode & std::ios_base::binary) {
    cmode += L"b";
  } else {
    cmode += L"t";
  }
  return cmode;
};

#  endif

template <typename CharType, typename Traits = std::char_traits<CharType> >
class basic_efilebuf
{
public:
#  if defined(_MSC_VER)
  typedef basic_filebuf<CharType, Traits> internal_buffer_type;
#  else
  typedef __gnu_cxx::stdio_filebuf<CharType, Traits> internal_buffer_type;
#  endif

  basic_efilebuf()
    : file_(0)
  {
    buf_ = 0;
  }

  bool _open(char const* file_name, std::ios_base::openmode mode)
  {
    if (_is_open() || file_) {
      return false;
    }
#  if defined(_MSC_VER)
    bool const success = buf_->open(file_name, mode) != 0;
#  else
    std::wstring const wstr = Encoding::ToWindowsExtendedPath(file_name);
    bool success = false;
    std::wstring cmode = getcmode(mode);
    file_ = _wfopen(wstr.c_str(), cmode.c_str());
    if (file_) {
      if (buf_) {
        delete buf_;
      }
      buf_ = new internal_buffer_type(file_, mode);
      success = true;
    }
#  endif
    return success;
  }

  bool _is_open()
  {
    if (!buf_) {
      return false;
    }
    return buf_->is_open();
  }

  bool _is_open() const
  {
    if (!buf_) {
      return false;
    }
    return buf_->is_open();
  }

  bool _close()
  {
    bool success = false;
    if (buf_) {
      success = buf_->close() != 0;
#  if !defined(_MSC_VER)
      if (file_) {
        success = fclose(file_) == 0 ? success : false;
        file_ = 0;
      }
#  endif
    }
    return success;
  }

  static void _set_state(bool success, std::basic_ios<CharType, Traits>* ios,
                         basic_efilebuf* efilebuf)
  {
#  if !defined(_MSC_VER)
    ios->rdbuf(efilebuf->buf_);
#  else
    static_cast<void>(efilebuf);
#  endif
    if (!success) {
      ios->setstate(std::ios_base::failbit);
    } else {
      ios->clear();
    }
  }

  ~basic_efilebuf()
  {
    if (buf_) {
      delete buf_;
    }
  }

protected:
  internal_buffer_type* buf_;
  FILE* file_;
};

template <typename CharType, typename Traits = std::char_traits<CharType> >
class basic_fstream
  : public std::basic_iostream<CharType, Traits>
  , public basic_efilebuf<CharType, Traits>
{
public:
  typedef typename basic_efilebuf<CharType, Traits>::internal_buffer_type
    internal_buffer_type;
  typedef std::basic_iostream<CharType, Traits> internal_stream_type;

  basic_fstream()
    : internal_stream_type(new internal_buffer_type())
  {
    this->buf_ =
      static_cast<internal_buffer_type*>(internal_stream_type::rdbuf());
  }
  explicit basic_fstream(char const* file_name,
                         std::ios_base::openmode mode = std::ios_base::in |
                           std::ios_base::out)
    : internal_stream_type(new internal_buffer_type())
  {
    this->buf_ =
      static_cast<internal_buffer_type*>(internal_stream_type::rdbuf());
    open(file_name, mode);
  }

  void open(char const* file_name,
            std::ios_base::openmode mode = std::ios_base::in |
              std::ios_base::out)
  {
    this->_set_state(this->_open(file_name, mode), this, this);
  }

  bool is_open() { return this->_is_open(); }

  void close() { this->_set_state(this->_close(), this, this); }

  using basic_efilebuf<CharType, Traits>::_is_open;

  internal_buffer_type* rdbuf() const { return this->buf_; }

  ~basic_fstream() cmsys_FStream_NOEXCEPT { close(); }
};

template <typename CharType, typename Traits = std::char_traits<CharType> >
class basic_ifstream
  : public std::basic_istream<CharType, Traits>
  , public basic_efilebuf<CharType, Traits>
{
public:
  typedef typename basic_efilebuf<CharType, Traits>::internal_buffer_type
    internal_buffer_type;
  typedef std::basic_istream<CharType, Traits> internal_stream_type;

  basic_ifstream()
    : internal_stream_type(new internal_buffer_type())
  {
    this->buf_ =
      static_cast<internal_buffer_type*>(internal_stream_type::rdbuf());
  }
  explicit basic_ifstream(char const* file_name,
                          std::ios_base::openmode mode = std::ios_base::in)
    : internal_stream_type(new internal_buffer_type())
  {
    this->buf_ =
      static_cast<internal_buffer_type*>(internal_stream_type::rdbuf());
    open(file_name, mode);
  }

  void open(char const* file_name,
            std::ios_base::openmode mode = std::ios_base::in)
  {
    mode = mode | std::ios_base::in;
    this->_set_state(this->_open(file_name, mode), this, this);
  }

  bool is_open() { return this->_is_open(); }

  void close() { this->_set_state(this->_close(), this, this); }

  using basic_efilebuf<CharType, Traits>::_is_open;

  internal_buffer_type* rdbuf() const { return this->buf_; }

  ~basic_ifstream() cmsys_FStream_NOEXCEPT { close(); }
};

template <typename CharType, typename Traits = std::char_traits<CharType> >
class basic_ofstream
  : public std::basic_ostream<CharType, Traits>
  , public basic_efilebuf<CharType, Traits>
{
  using basic_efilebuf<CharType, Traits>::_is_open;

public:
  typedef typename basic_efilebuf<CharType, Traits>::internal_buffer_type
    internal_buffer_type;
  typedef std::basic_ostream<CharType, Traits> internal_stream_type;

  basic_ofstream()
    : internal_stream_type(new internal_buffer_type())
  {
    this->buf_ =
      static_cast<internal_buffer_type*>(internal_stream_type::rdbuf());
  }
  explicit basic_ofstream(char const* file_name,
                          std::ios_base::openmode mode = std::ios_base::out)
    : internal_stream_type(new internal_buffer_type())
  {
    this->buf_ =
      static_cast<internal_buffer_type*>(internal_stream_type::rdbuf());
    open(file_name, mode);
  }
  void open(char const* file_name,
            std::ios_base::openmode mode = std::ios_base::out)
  {
    mode = mode | std::ios_base::out;
    this->_set_state(this->_open(file_name, mode), this, this);
  }

  void close() { this->_set_state(this->_close(), this, this); }

  bool is_open() { return this->_is_open(); }

  internal_buffer_type* rdbuf() const { return this->buf_; }

  ~basic_ofstream() cmsys_FStream_NOEXCEPT { close(); }
};

typedef basic_fstream<char> fstream;
typedef basic_ifstream<char> ifstream;
typedef basic_ofstream<char> ofstream;

#  undef cmsys_FStream_NOEXCEPT
#else
using std::fstream;
using std::ofstream;
using std::ifstream;
#endif

namespace FStream {
enum BOM
{
  BOM_None,
  BOM_UTF8,
  BOM_UTF16BE,
  BOM_UTF16LE,
  BOM_UTF32BE,
  BOM_UTF32LE
};

// Read a BOM, if one exists.
// If a BOM exists, the stream is advanced to after the BOM.
// This function requires a seekable stream (but not a relative
// seekable stream).
cmsys_EXPORT BOM ReadBOM(std::istream& in);
}
}

#endif
