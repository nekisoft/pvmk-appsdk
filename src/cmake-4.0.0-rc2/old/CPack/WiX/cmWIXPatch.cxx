/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWIXPatch.h"

#include "../cmCPackGenerator.h"

cmWIXPatch::cmWIXPatch(cmCPackLog* logger)
  : Logger(logger)
{
}

bool cmWIXPatch::LoadFragments(std::string const& patchFilePath)
{
  cmWIXPatchParser parser(Fragments, Logger);
  if (!parser.ParseFile(patchFilePath.c_str())) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Failed parsing XML patch file: '" << patchFilePath << '\''
                                                     << std::endl);
    return false;
  }

  return true;
}

void cmWIXPatch::ApplyFragment(std::string const& id,
                               cmWIXSourceWriter& writer)
{
  auto i = Fragments.find(id);
  if (i == Fragments.end()) {
    return;
  }

  cmWIXPatchElement const& fragment = i->second;
  for (auto const& attr : fragment.attributes) {
    writer.AddAttribute(attr.first, attr.second);
  }
  this->ApplyElementChildren(fragment, writer);

  Fragments.erase(i);
}

void cmWIXPatch::ApplyElementChildren(cmWIXPatchElement const& element,
                                      cmWIXSourceWriter& writer)
{
  for (auto const& node : element.children) {
    switch (node->type()) {
      case cmWIXPatchNode::ELEMENT:
        ApplyElement(dynamic_cast<cmWIXPatchElement const&>(*node), writer);
        break;
      case cmWIXPatchNode::TEXT:
        writer.AddTextNode(dynamic_cast<cmWIXPatchText const&>(*node).text);
        break;
    }
  }
}

void cmWIXPatch::ApplyElement(cmWIXPatchElement const& element,
                              cmWIXSourceWriter& writer)
{
  writer.BeginElement(element.name);

  for (auto const& attr : element.attributes) {
    writer.AddAttribute(attr.first, attr.second);
  }

  this->ApplyElementChildren(element, writer);

  writer.EndElement(element.name);
}

bool cmWIXPatch::CheckForUnappliedFragments()
{
  std::string fragmentList;
  for (auto const& fragment : Fragments) {
    if (!fragmentList.empty()) {
      fragmentList += ", ";
    }

    fragmentList += '\'';
    fragmentList += fragment.first;
    fragmentList += '\'';
  }

  if (!fragmentList.empty()) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Some XML patch fragments did not have matching IDs: "
                    << fragmentList << std::endl);
    return false;
  }

  return true;
}
