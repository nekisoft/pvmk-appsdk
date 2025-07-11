/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWIXPatchParser.h"

#include <utility>

#include <cm/memory>
#include <cmext/string_view>

#include <cm3p/expat.h>

#include "../cmCPackGenerator.h"

cmWIXPatchNode::Type cmWIXPatchText::type()
{
  return cmWIXPatchNode::TEXT;
}

cmWIXPatchNode::Type cmWIXPatchElement::type()
{
  return cmWIXPatchNode::ELEMENT;
}

cmWIXPatchNode::~cmWIXPatchNode() = default;

cmWIXPatchElement::cmWIXPatchElement() = default;
cmWIXPatchElement::~cmWIXPatchElement() = default;

cmWIXPatchParser::cmWIXPatchParser(fragment_map_t& fragments,
                                   cmCPackLog* logger)
  : Logger(logger)
  , State(BEGIN_DOCUMENT)
  , Valid(true)
  , Fragments(fragments)
{
}

void cmWIXPatchParser::StartElement(std::string const& name, char const** atts)
{
  if (State == BEGIN_DOCUMENT) {
    if (name == "CPackWiXPatch"_s) {
      State = BEGIN_FRAGMENTS;
    } else {
      ReportValidationError("Expected root element 'CPackWiXPatch'");
    }
  } else if (State == BEGIN_FRAGMENTS) {
    if (name == "CPackWiXFragment"_s) {
      State = INSIDE_FRAGMENT;
      StartFragment(atts);
    } else {
      ReportValidationError("Expected 'CPackWixFragment' element");
    }
  } else if (State == INSIDE_FRAGMENT) {
    cmWIXPatchElement& parent = *ElementStack.back();

    auto element = cm::make_unique<cmWIXPatchElement>();

    element->name = name;

    for (size_t i = 0; atts[i]; i += 2) {
      std::string key = atts[i];
      std::string value = atts[i + 1];

      element->attributes[key] = value;
    }

    ElementStack.push_back(element.get());
    parent.children.push_back(std::move(element));
  }
}

void cmWIXPatchParser::StartFragment(char const** attributes)
{
  cmWIXPatchElement* new_element = nullptr;
  /* find the id of for fragment */
  for (size_t i = 0; attributes[i]; i += 2) {
    std::string const key = attributes[i];
    std::string const value = attributes[i + 1];

    if (key == "Id"_s) {
      if (Fragments.find(value) != Fragments.end()) {
        std::ostringstream tmp;
        tmp << "Invalid reuse of 'CPackWixFragment' 'Id': " << value;
        ReportValidationError(tmp.str());
      }

      new_element = &Fragments[value];
      ElementStack.push_back(new_element);
    }
  }

  /* add any additional attributes for the fragment */
  if (!new_element) {
    ReportValidationError("No 'Id' specified for 'CPackWixFragment' element");
  } else {
    for (size_t i = 0; attributes[i]; i += 2) {
      std::string const key = attributes[i];
      std::string const value = attributes[i + 1];

      if (key != "Id"_s) {
        new_element->attributes[key] = value;
      }
    }
  }
}

void cmWIXPatchParser::EndElement(std::string const& name)
{
  if (State == INSIDE_FRAGMENT) {
    if (name == "CPackWiXFragment"_s) {
      State = BEGIN_FRAGMENTS;
      ElementStack.clear();
    } else {
      ElementStack.pop_back();
    }
  }
}

void cmWIXPatchParser::CharacterDataHandler(char const* data, int length)
{
  char const* whitespace = "\x20\x09\x0d\x0a";

  if (State == INSIDE_FRAGMENT) {
    cmWIXPatchElement& parent = *ElementStack.back();

    std::string text(data, length);

    std::string::size_type first = text.find_first_not_of(whitespace);
    std::string::size_type last = text.find_last_not_of(whitespace);

    if (first != std::string::npos && last != std::string::npos) {
      auto text_node = cm::make_unique<cmWIXPatchText>();
      text_node->text = text.substr(first, last - first + 1);

      parent.children.push_back(std::move(text_node));
    }
  }
}

void cmWIXPatchParser::ReportError(int line, int column, char const* msg)
{
  cmCPackLogger(cmCPackLog::LOG_ERROR,
                "Error while processing XML patch file at "
                  << line << ':' << column << ":  " << msg << std::endl);
  Valid = false;
}

void cmWIXPatchParser::ReportValidationError(std::string const& message)
{
  ReportError(
    XML_GetCurrentLineNumber(static_cast<XML_Parser>(this->Parser)),
    XML_GetCurrentColumnNumber(static_cast<XML_Parser>(this->Parser)),
    message.c_str());
}

bool cmWIXPatchParser::IsValid() const
{
  return Valid;
}
