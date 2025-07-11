/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_Directory_hxx
#define cmsys_Directory_hxx

#include <cmsys/Configure.h>
#include <cmsys/Status.hxx>

#include <cstddef>
#include <string>

namespace cmsys {

class DirectoryInternals;

/** \class Directory
 * \brief Portable directory/filename traversal.
 *
 * Directory provides a portable way of finding the names of the files
 * in a system directory.
 *
 * Directory currently works with Windows and Unix operating systems.
 */
class cmsys_EXPORT Directory
{
public:
  Directory();
  Directory(Directory&& other);
  Directory(Directory const&) = delete;
  Directory& operator=(Directory const&) = delete;
  Directory& operator=(Directory&& other);
  bool operator==(Directory const&) = delete;
  ~Directory();

  /**
   * Load the specified directory and load the names of the files
   * in that directory.
   */
  Status Load(std::string const&, std::string* errorMessage = nullptr);

  /**
   * Return the number of files in the current directory.
   */
  unsigned long GetNumberOfFiles() const;

  /**
   * Return the number of files in the specified directory.
   * A higher performance static method.
   */
  static unsigned long GetNumberOfFilesInDirectory(
    std::string const&, std::string* errorMessage = nullptr);

  /**
   * Return the file at the given index, the indexing is 0 based
   */
  char const* GetFile(unsigned long) const;

  /**
   * Return the name of the file at the given 0-based index.
   */
  std::string const& GetFileName(std::size_t i) const;

  /**
   * Return the absolute path to the file at the given 0-based index.
   */
  std::string GetFilePath(std::size_t i) const;

  /**
   * Return whether the file at the given 0-based index is a directory.
   */
  bool FileIsDirectory(std::size_t i) const;

  /**
   * Return whether the file at the given 0-based index is a symlink.
   */
  bool FileIsSymlink(std::size_t i) const;

  /**
   * Return the path to Open'ed directory
   */
  char const* GetPath() const;

  /**
   * Clear the internal structure. Used internally at beginning of Load(...)
   * to clear the cache.
   */
  void Clear();

private:
  // Private implementation details.
  DirectoryInternals* Internal;
}; // End Class: Directory

} // namespace cmsys

#endif
