/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#pragma once

#include <map>
#include <memory>
#include <vector>

#include "../cmCPackLog.h"
#include "cmXMLParser.h"

struct cmWIXPatchNode
{
  enum Type
  {
    TEXT,
    ELEMENT
  };

  virtual ~cmWIXPatchNode();

  virtual Type type() = 0;
};

struct cmWIXPatchText : public cmWIXPatchNode
{
  virtual Type type();

  std::string text;
};

struct cmWIXPatchElement : cmWIXPatchNode
{
  virtual Type type();

  cmWIXPatchElement();

  cmWIXPatchElement(cmWIXPatchElement const&) = delete;
  cmWIXPatchElement const& operator=(cmWIXPatchElement const&) = delete;

  ~cmWIXPatchElement();

  using child_list_t = std::vector<std::unique_ptr<cmWIXPatchNode>>;
  using attributes_t = std::map<std::string, std::string>;

  std::string name;
  child_list_t children;
  attributes_t attributes;
};

/** \class cmWIXPatchParser
 * \brief Helper class that parses XML patch files (CPACK_WIX_PATCH_FILE)
 */
class cmWIXPatchParser : public cmXMLParser
{
public:
  using fragment_map_t = std::map<std::string, cmWIXPatchElement>;

  cmWIXPatchParser(fragment_map_t& Fragments, cmCPackLog* logger);

private:
  virtual void StartElement(std::string const& name, char const** atts);

  void StartFragment(char const** attributes);

  virtual void EndElement(std::string const& name);

  virtual void CharacterDataHandler(char const* data, int length);

  virtual void ReportError(int line, int column, char const* msg);

  void ReportValidationError(std::string const& message);

  bool IsValid() const;

  cmCPackLog* Logger;

  enum ParserState
  {
    BEGIN_DOCUMENT,
    BEGIN_FRAGMENTS,
    INSIDE_FRAGMENT
  };

  ParserState State;

  bool Valid;

  fragment_map_t& Fragments;

  std::vector<cmWIXPatchElement*> ElementStack;
};
