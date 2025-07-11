#if 0

/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmXCode21Object.h"

#include <ostream>
#include <string>
#include <utility>

#include "cmSystemTools.h"

cmXCode21Object::cmXCode21Object(PBXType ptype, Type type, std::string id)
  : cmXCodeObject(ptype, type, std::move(id))
{
  this->Version = 21;
}

void cmXCode21Object::PrintComment(std::ostream& out)
{
  if (this->Comment.empty()) {
    cmXCodeObject* n = this->GetAttribute("name");
    if (n) {
      this->Comment = n->GetString();
      cmSystemTools::ReplaceString(this->Comment, "\"", "");
    }
  }
  if (this->Comment.empty()) {
    return;
  }
  out << " /* " << this->Comment << " */";
}

void cmXCode21Object::PrintList(
  std::vector<std::unique_ptr<cmXCodeObject>> const& v, std::ostream& out,
  PBXType t)
{
  bool hasOne = false;
  for (auto const& obj : v) {
    if (obj->GetType() == OBJECT && obj->GetIsA() == t) {
      hasOne = true;
      break;
    }
  }
  if (!hasOne) {
    return;
  }
  out << "\n/* Begin " << PBXTypeNames[t] << " section */\n";
  for (auto const& obj : v) {
    if (obj->GetType() == OBJECT && obj->GetIsA() == t) {
      obj->Print(out);
    }
  }
  out << "/* End " << PBXTypeNames[t] << " section */\n";
}

void cmXCode21Object::PrintList(
  std::vector<std::unique_ptr<cmXCodeObject>> const& v, std::ostream& out)
{
  cmXCodeObject::Indent(1, out);
  out << "objects = {\n";
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXAggregateTarget);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXBuildFile);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXBuildStyle);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXContainerItemProxy);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXFileReference);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXFrameworksBuildPhase);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXGroup);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXHeadersBuildPhase);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXNativeTarget);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXProject);
  cmXCode21Object::PrintList(v, out,
                             cmXCode21Object::PBXShellScriptBuildPhase);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXResourcesBuildPhase);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXSourcesBuildPhase);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXCopyFilesBuildPhase);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXApplicationReference);
  cmXCode21Object::PrintList(v, out,
                             cmXCode21Object::PBXExecutableFileReference);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXLibraryReference);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXToolTarget);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXLibraryTarget);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::PBXTargetDependency);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::XCBuildConfiguration);
  cmXCode21Object::PrintList(v, out, cmXCode21Object::XCConfigurationList);
  cmXCodeObject::Indent(1, out);
  out << "};\n";
}

#endif //0
