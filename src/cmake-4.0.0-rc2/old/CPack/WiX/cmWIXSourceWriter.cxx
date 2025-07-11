/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifdef __MINGW32__

#include "cmWIXSourceWriter.h"

#include <windows.h>

#include "../cmCPackGenerator.h"
#include "cmCryptoHash.h"
#include "cmUuid.h"

cmWIXSourceWriter::cmWIXSourceWriter(unsigned long wixVersion,
                                     cmCPackLog* logger,
                                     std::string const& filename,
                                     GuidType componentGuidType,
                                     RootElementType rootElementType)
  : WixVersion(wixVersion)
  , Logger(logger)
  , File(filename.c_str())
  , State(DEFAULT)
  , SourceFilename(filename)
  , ComponentGuidType(componentGuidType)
{
  WriteXMLDeclaration();

  if (rootElementType == INCLUDE_ELEMENT_ROOT) {
    BeginElement("Include");
  } else {
    BeginElement("Wix");
  }

  if (this->WixVersion >= 4) {
    AddAttribute("xmlns", "http://wixtoolset.org/schemas/v4/wxs");
  } else {
    AddAttribute("xmlns", "http://schemas.microsoft.com/wix/2006/wi");
  }
}

cmWIXSourceWriter::~cmWIXSourceWriter()
{
  if (Elements.size() > 1) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  Elements.size() - 1
                    << " WiX elements were still open when closing '"
                    << SourceFilename << '\'' << std::endl);
    return;
  }

  EndElement(Elements.back());
}

void cmWIXSourceWriter::BeginElement_StandardDirectory()
{
  if (this->WixVersion >= 4) {
    BeginElement("StandardDirectory");
  } else {
    BeginElement("Directory");
  }
}

void cmWIXSourceWriter::EndElement_StandardDirectory()
{
  if (this->WixVersion >= 4) {
    EndElement("StandardDirectory");
  } else {
    EndElement("Directory");
  }
}

void cmWIXSourceWriter::BeginElement(std::string const& name)
{
  if (State == BEGIN) {
    File << '>';
  }

  File << '\n';
  Indent(Elements.size());
  File << '<' << name;

  Elements.push_back(name);
  State = BEGIN;
}

void cmWIXSourceWriter::EndElement(std::string const& name)
{
  if (Elements.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "can not end WiX element with no open elements in '"
                    << SourceFilename << '\'' << std::endl);
    return;
  }

  if (Elements.back() != name) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "WiX element <"
                    << Elements.back() << "> can not be closed by </" << name
                    << "> in '" << SourceFilename << '\'' << std::endl);
    return;
  }

  if (State == DEFAULT) {
    File << '\n';
    Indent(Elements.size() - 1);
    File << "</" << Elements.back() << '>';
  } else {
    File << "/>";
  }

  Elements.pop_back();
  State = DEFAULT;
}

void cmWIXSourceWriter::AddTextNode(std::string const& text)
{
  if (State == BEGIN) {
    File << '>';
  }

  if (Elements.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "can not add text without open WiX element in '"
                    << SourceFilename << '\'' << std::endl);
    return;
  }

  File << cmWIXSourceWriter::EscapeAttributeValue(text);
  State = DEFAULT;
}

void cmWIXSourceWriter::AddProcessingInstruction(std::string const& target,
                                                 std::string const& content)
{
  if (State == BEGIN) {
    File << '>';
  }

  File << '\n';
  Indent(Elements.size());
  File << "<?" << target << ' ' << content << "?>";

  State = DEFAULT;
}

void cmWIXSourceWriter::AddAttribute(std::string const& key,
                                     std::string const& value)
{
  File << ' ' << key << "=\"" << EscapeAttributeValue(value) << '"';
}

void cmWIXSourceWriter::AddAttributeUnlessEmpty(std::string const& key,
                                                std::string const& value)
{
  if (!value.empty()) {
    AddAttribute(key, value);
  }
}

std::string cmWIXSourceWriter::CreateGuidFromComponentId(
  std::string const& componentId)
{
  std::string guid = "*";
  if (this->ComponentGuidType == CMAKE_GENERATED_GUID) {
    cmCryptoHash hasher(cmCryptoHash::AlgoMD5);
    std::string md5 = hasher.HashString(componentId);
    cmUuid uuid;
    std::vector<unsigned char> ns;
    guid = uuid.FromMd5(ns, md5);
  }
  return guid;
}

void cmWIXSourceWriter::WriteXMLDeclaration()
{
  File << R"(<?xml version="1.0" encoding="UTF-8"?>)" << std::endl;
}

void cmWIXSourceWriter::Indent(size_t count)
{
  for (size_t i = 0; i < count; ++i) {
    File << "    ";
  }
}

std::string cmWIXSourceWriter::EscapeAttributeValue(std::string const& value)
{
  std::string result;
  result.reserve(value.size());

  for (char c : value) {
    switch (c) {
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '&':
        result += "&amp;";
        break;
      case '"':
        result += "&quot;";
        break;
      default:
        result += c;
        break;
    }
  }

  return result;
}

#endif //#ifdef __MINGW32__
