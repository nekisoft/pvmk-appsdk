/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_ConsoleBuf_hxx
#define cmsys_ConsoleBuf_hxx

#include <cmsys/Configure.hxx>

#include <cmsys/Encoding.hxx>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>

#if defined(_WIN32)
#  include <windows.h>
#  if __cplusplus >= 201103L
#    include <system_error>
#  endif
#endif

namespace cmsys {
#if defined(_WIN32)

template <class CharT, class Traits = std::char_traits<CharT> >
class BasicConsoleBuf : public std::basic_streambuf<CharT, Traits>
{
public:
  typedef typename Traits::int_type int_type;
  typedef typename Traits::char_type char_type;

  class Manager
  {
  public:
    Manager(std::basic_ios<CharT, Traits>& ios, bool const err = false)
      : m_consolebuf(0)
    {
      m_ios = &ios;
      try {
        m_consolebuf = new BasicConsoleBuf<CharT, Traits>(err);
        m_streambuf = m_ios->rdbuf(m_consolebuf);
      } catch (std::runtime_error const& ex) {
        std::cerr << "Failed to create ConsoleBuf!" << std::endl
                  << ex.what() << std::endl;
      };
    }

    BasicConsoleBuf<CharT, Traits>* GetConsoleBuf() { return m_consolebuf; }

    void SetUTF8Pipes()
    {
      if (m_consolebuf) {
        m_consolebuf->input_pipe_codepage = CP_UTF8;
        m_consolebuf->output_pipe_codepage = CP_UTF8;
        m_consolebuf->activateCodepageChange();
      }
    }

    ~Manager()
    {
      if (m_consolebuf) {
        delete m_consolebuf;
        m_ios->rdbuf(m_streambuf);
      }
    }

  private:
    std::basic_ios<CharT, Traits>* m_ios;
    std::basic_streambuf<CharT, Traits>* m_streambuf;
    BasicConsoleBuf<CharT, Traits>* m_consolebuf;
  };

  BasicConsoleBuf(bool const err = false)
    : flush_on_newline(true)
    , input_pipe_codepage(0)
    , output_pipe_codepage(0)
    , input_file_codepage(CP_UTF8)
    , output_file_codepage(CP_UTF8)
    , m_consolesCodepage(0)
  {
    m_hInput = ::GetStdHandle(STD_INPUT_HANDLE);
    checkHandle(true, "STD_INPUT_HANDLE");
    if (!setActiveInputCodepage()) {
      throw std::runtime_error("setActiveInputCodepage failed!");
    }
    m_hOutput = err ? ::GetStdHandle(STD_ERROR_HANDLE)
                    : ::GetStdHandle(STD_OUTPUT_HANDLE);
    checkHandle(false, err ? "STD_ERROR_HANDLE" : "STD_OUTPUT_HANDLE");
    if (!setActiveOutputCodepage()) {
      throw std::runtime_error("setActiveOutputCodepage failed!");
    }
    _setg();
    _setp();
  }

  ~BasicConsoleBuf() throw() { sync(); }

  bool activateCodepageChange()
  {
    return setActiveInputCodepage() && setActiveOutputCodepage();
  }

protected:
  virtual int sync()
  {
    bool success = true;
    if (m_hInput && m_isConsoleInput &&
        ::FlushConsoleInputBuffer(m_hInput) == 0) {
      success = false;
    }
    if (m_hOutput && !m_obuffer.empty()) {
      std::wstring const wbuffer = getBuffer(m_obuffer);
      if (m_isConsoleOutput) {
        DWORD charsWritten;
        success =
          ::WriteConsoleW(m_hOutput, wbuffer.c_str(), (DWORD)wbuffer.size(),
                          &charsWritten, nullptr) == 0
          ? false
          : true;
      } else {
        DWORD bytesWritten;
        std::string buffer;
        success = encodeOutputBuffer(wbuffer, buffer);
        if (success) {
          success =
            ::WriteFile(m_hOutput, buffer.c_str(), (DWORD)buffer.size(),
                        &bytesWritten, nullptr) == 0
            ? false
            : true;
        }
      }
    }
    m_ibuffer.clear();
    m_obuffer.clear();
    _setg();
    _setp();
    return success ? 0 : -1;
  }

  virtual int_type underflow()
  {
    if (this->gptr() >= this->egptr()) {
      if (!m_hInput) {
        _setg(true);
        return Traits::eof();
      }
      if (m_isConsoleInput) {
        // ReadConsole doesn't tell if there's more input available
        // don't support reading more characters than this
        wchar_t wbuffer[8192];
        DWORD charsRead;
        if (ReadConsoleW(m_hInput, wbuffer,
                         (sizeof(wbuffer) / sizeof(wbuffer[0])), &charsRead,
                         nullptr) == 0 ||
            charsRead == 0) {
          _setg(true);
          return Traits::eof();
        }
        setBuffer(std::wstring(wbuffer, charsRead), m_ibuffer);
      } else {
        std::wstring wbuffer;
        std::string strbuffer;
        DWORD bytesRead;
        LARGE_INTEGER size;
        if (GetFileSizeEx(m_hInput, &size) == 0) {
          _setg(true);
          return Traits::eof();
        }
        char* buffer = new char[size.LowPart];
        while (ReadFile(m_hInput, buffer, size.LowPart, &bytesRead, nullptr) ==
               0) {
          if (GetLastError() == ERROR_MORE_DATA) {
            strbuffer += std::string(buffer, bytesRead);
            continue;
          }
          _setg(true);
          delete[] buffer;
          return Traits::eof();
        }
        if (bytesRead > 0) {
          strbuffer += std::string(buffer, bytesRead);
        }
        delete[] buffer;
        if (!decodeInputBuffer(strbuffer, wbuffer)) {
          _setg(true);
          return Traits::eof();
        }
        setBuffer(wbuffer, m_ibuffer);
      }
      _setg();
    }
    return Traits::to_int_type(*this->gptr());
  }

  virtual int_type overflow(int_type ch = Traits::eof())
  {
    if (!Traits::eq_int_type(ch, Traits::eof())) {
      char_type chr = Traits::to_char_type(ch);
      m_obuffer += chr;
      if ((flush_on_newline && Traits::eq(chr, '\n')) ||
          Traits::eq_int_type(ch, 0x00)) {
        sync();
      }
      return ch;
    }
    sync();
    return Traits::eof();
  }

public:
  bool flush_on_newline;
  UINT input_pipe_codepage;
  UINT output_pipe_codepage;
  UINT input_file_codepage;
  UINT output_file_codepage;

private:
  HANDLE m_hInput;
  HANDLE m_hOutput;
  std::basic_string<char_type> m_ibuffer;
  std::basic_string<char_type> m_obuffer;
  bool m_isConsoleInput;
  bool m_isConsoleOutput;
  UINT m_activeInputCodepage;
  UINT m_activeOutputCodepage;
  UINT m_consolesCodepage;
  void checkHandle(bool input, std::string handleName)
  {
    if ((input && m_hInput == INVALID_HANDLE_VALUE) ||
        (!input && m_hOutput == INVALID_HANDLE_VALUE)) {
      std::string errmsg =
        "GetStdHandle(" + handleName + ") returned INVALID_HANDLE_VALUE";
#  if __cplusplus >= 201103L
      throw std::system_error(::GetLastError(), std::system_category(),
                              errmsg);
#  else
      throw std::runtime_error(errmsg);
#  endif
    }
  }
  UINT getConsolesCodepage()
  {
    if (!m_consolesCodepage) {
      m_consolesCodepage = GetConsoleCP();
      if (!m_consolesCodepage) {
        m_consolesCodepage = GetACP();
      }
    }
    return m_consolesCodepage;
  }
  bool setActiveInputCodepage()
  {
    m_isConsoleInput = false;
    switch (GetFileType(m_hInput)) {
      case FILE_TYPE_DISK:
        m_activeInputCodepage = input_file_codepage;
        break;
      case FILE_TYPE_CHAR:
        // Check for actual console.
        DWORD consoleMode;
        m_isConsoleInput =
          GetConsoleMode(m_hInput, &consoleMode) == 0 ? false : true;
        if (m_isConsoleInput) {
          break;
        }
        cmsys_FALLTHROUGH;
      case FILE_TYPE_PIPE:
        m_activeInputCodepage = input_pipe_codepage;
        break;
      default:
        return false;
    }
    if (!m_isConsoleInput && m_activeInputCodepage == 0) {
      m_activeInputCodepage = getConsolesCodepage();
    }
    return true;
  }
  bool setActiveOutputCodepage()
  {
    m_isConsoleOutput = false;
    switch (GetFileType(m_hOutput)) {
      case FILE_TYPE_DISK:
        m_activeOutputCodepage = output_file_codepage;
        break;
      case FILE_TYPE_CHAR:
        // Check for actual console.
        DWORD consoleMode;
        m_isConsoleOutput =
          GetConsoleMode(m_hOutput, &consoleMode) == 0 ? false : true;
        if (m_isConsoleOutput) {
          break;
        }
        cmsys_FALLTHROUGH;
      case FILE_TYPE_PIPE:
        m_activeOutputCodepage = output_pipe_codepage;
        break;
      default:
        return false;
    }
    if (!m_isConsoleOutput && m_activeOutputCodepage == 0) {
      m_activeOutputCodepage = getConsolesCodepage();
    }
    return true;
  }
  void _setg(bool empty = false)
  {
    if (!empty) {
      this->setg((char_type*)m_ibuffer.data(), (char_type*)m_ibuffer.data(),
                 (char_type*)m_ibuffer.data() + m_ibuffer.size());
    } else {
      this->setg((char_type*)m_ibuffer.data(),
                 (char_type*)m_ibuffer.data() + m_ibuffer.size(),
                 (char_type*)m_ibuffer.data() + m_ibuffer.size());
    }
  }
  void _setp()
  {
    this->setp((char_type*)m_obuffer.data(),
               (char_type*)m_obuffer.data() + m_obuffer.size());
  }
  bool encodeOutputBuffer(std::wstring const wbuffer, std::string& buffer)
  {
    if (wbuffer.size() == 0) {
      buffer = std::string();
      return true;
    }
    int const length =
      WideCharToMultiByte(m_activeOutputCodepage, 0, wbuffer.c_str(),
                          (int)wbuffer.size(), nullptr, 0, nullptr, nullptr);
    char* buf = new char[length];
    bool const success =
      WideCharToMultiByte(m_activeOutputCodepage, 0, wbuffer.c_str(),
                          (int)wbuffer.size(), buf, length, nullptr,
                          nullptr) > 0
      ? true
      : false;
    buffer = std::string(buf, length);
    delete[] buf;
    return success;
  }
  bool decodeInputBuffer(std::string const buffer, std::wstring& wbuffer)
  {
    size_t length = buffer.length();
    if (length == 0) {
      wbuffer = std::wstring();
      return true;
    }
    int actualCodepage = m_activeInputCodepage;
    char const BOM_UTF8[] = { char(0xEF), char(0xBB), char(0xBF) };
    char const* data = buffer.data();
    size_t const BOMsize = sizeof(BOM_UTF8);
    if (length >= BOMsize && std::memcmp(data, BOM_UTF8, BOMsize) == 0) {
      // PowerShell uses UTF-8 with BOM for pipes
      actualCodepage = CP_UTF8;
      data += BOMsize;
      length -= BOMsize;
    }
    size_t const wlength = static_cast<size_t>(MultiByteToWideChar(
      actualCodepage, 0, data, static_cast<int>(length), nullptr, 0));
    wchar_t* wbuf = new wchar_t[wlength];
    bool const success =
      MultiByteToWideChar(actualCodepage, 0, data, static_cast<int>(length),
                          wbuf, static_cast<int>(wlength)) > 0
      ? true
      : false;
    wbuffer = std::wstring(wbuf, wlength);
    delete[] wbuf;
    return success;
  }
  std::wstring getBuffer(std::basic_string<char> const buffer)
  {
    return Encoding::ToWide(buffer);
  }
  std::wstring getBuffer(std::basic_string<wchar_t> const buffer)
  {
    return buffer;
  }
  void setBuffer(std::wstring const wbuffer, std::basic_string<char>& target)
  {
    target = Encoding::ToNarrow(wbuffer);
  }
  void setBuffer(std::wstring const wbuffer,
                 std::basic_string<wchar_t>& target)
  {
    target = wbuffer;
  }

}; // BasicConsoleBuf class

typedef BasicConsoleBuf<char> ConsoleBuf;
typedef BasicConsoleBuf<wchar_t> WConsoleBuf;

#endif
} // KWSYS_NAMESPACE

#endif
