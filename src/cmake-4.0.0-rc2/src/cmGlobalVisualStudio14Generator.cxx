#ifdef __MINGW32__

/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudio14Generator.h"

#include <cstring>
#include <sstream>

#include <cm/vector>
#include <cmext/string_view>

#include "cmGlobalGenerator.h"
#include "cmGlobalGeneratorFactory.h"
#include "cmGlobalVisualStudioGenerator.h"
#include "cmMakefile.h"
#include "cmMessageType.h"
#include "cmPolicies.h"
#include "cmStringAlgorithms.h"
#include "cmSystemTools.h"
#include "cmValue.h"

static char const vs14generatorName[] = "Visual Studio 14 2015";

// Map generator name without year to name with year.
static char const* cmVS14GenName(std::string const& name, std::string& genName)
{
  if (strncmp(name.c_str(), vs14generatorName,
              sizeof(vs14generatorName) - 6) != 0) {
    return nullptr;
  }
  char const* p = name.c_str() + sizeof(vs14generatorName) - 6;
  if (cmHasLiteralPrefix(p, " 2015")) {
    p += 5;
  }
  genName = std::string(vs14generatorName) + p;
  return p;
}

class cmGlobalVisualStudio14Generator::Factory
  : public cmGlobalGeneratorFactory
{
public:
  std::unique_ptr<cmGlobalGenerator> CreateGlobalGenerator(
    std::string const& name, cmake* cm) const override
  {
    std::string genName;
    char const* p = cmVS14GenName(name, genName);
    if (!p) {
      return std::unique_ptr<cmGlobalGenerator>();
    }
    if (!*p) {
      return std::unique_ptr<cmGlobalGenerator>(
        new cmGlobalVisualStudio14Generator(cm, genName));
    }
    return std::unique_ptr<cmGlobalGenerator>();
  }

  cmDocumentationEntry GetDocumentation() const override
  {
    return { cmStrCat(vs14generatorName, " [arch]"),
             "Generates Visual Studio 2015 project files.  "
             "Optional [arch] can be \"Win64\" or \"ARM\"." };
  }

  std::vector<std::string> GetGeneratorNames() const override
  {
    std::vector<std::string> names;
    names.push_back(vs14generatorName);
    return names;
  }

  bool SupportsToolset() const override { return true; }
  bool SupportsPlatform() const override { return true; }

  std::vector<std::string> GetKnownPlatforms() const override
  {
    std::vector<std::string> platforms;
    platforms.emplace_back("x64");
    platforms.emplace_back("Win32");
    platforms.emplace_back("ARM");
    return platforms;
  }

  std::string GetDefaultPlatformName() const override { return "Win32"; }
};

std::unique_ptr<cmGlobalGeneratorFactory>
cmGlobalVisualStudio14Generator::NewFactory()
{
  return std::unique_ptr<cmGlobalGeneratorFactory>(new Factory);
}

cmGlobalVisualStudio14Generator::cmGlobalVisualStudio14Generator(
  cmake* cm, std::string const& name)
  : cmGlobalVisualStudio12Generator(cm, name)
{
  std::string vc14Express;
  this->ExpressEdition = cmSystemTools::ReadRegistryValue(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\14.0\\Setup\\VC;"
    "ProductDir",
    vc14Express, cmSystemTools::KeyWOW64_32);
  this->DefaultPlatformToolset = "v140";
  this->DefaultAndroidToolset = "Clang_3_8";
  this->DefaultCLFlagTableName = "v140";
  this->DefaultCSharpFlagTableName = "v140";
  this->DefaultLibFlagTableName = "v14";
  this->DefaultLinkFlagTableName = "v140";
  this->DefaultMasmFlagTableName = "v14";
  this->DefaultRCFlagTableName = "v14";
  this->Version = VSVersion::VS14;
}

bool cmGlobalVisualStudio14Generator::MatchesGeneratorName(
  std::string const& name) const
{
  std::string genName;
  if (cmVS14GenName(name, genName)) {
    return genName == this->GetName();
  }
  return false;
}

bool cmGlobalVisualStudio14Generator::InitializePlatformWindows(cmMakefile* mf)
{
  // If a Windows SDK version is explicitly requested, search for it.
  if (this->GeneratorPlatformVersion) {
    std::string const& version = *this->GeneratorPlatformVersion;

    // VS 2019 and above support specifying plain "10.0".
    if (version == "10.0"_s) {
      if (this->Version >= VSVersion::VS16) {
        this->SetWindowsTargetPlatformVersion("10.0", mf);
        return true;
      }
      /* clang-format off */
      mf->IssueMessage(MessageType::FATAL_ERROR, cmStrCat(
          "Generator\n"
          "  ", this->GetName(), "\n"
          "given platform specification containing a\n"
          "  version=10.0\n"
          "field.  The value 10.0 is only supported by VS 2019 and above.\n"
          ));
      /* clang-format on */
      return false;
    }

    if (cmHasLiteralPrefix(version, "10.0.")) {
      return this->SelectWindows10SDK(mf);
    }

    if (version == "8.1"_s) {
      if (this->IsWin81SDKInstalled()) {
        this->SetWindowsTargetPlatformVersion("8.1", mf);
        return true;
      }
      /* clang-format off */
      mf->IssueMessage(MessageType::FATAL_ERROR, cmStrCat(
          "Generator\n"
          "  ", this->GetName(), "\n"
          "given platform specification containing a\n"
          "  version=8.1\n"
          "field, but the Windows 8.1 SDK is not installed.\n"
          ));
      /* clang-format on */
      return false;
    }

    if (version.empty()) {
      /* clang-format off */
      mf->IssueMessage(MessageType::FATAL_ERROR, cmStrCat(
          "Generator\n"
          "  ", this->GetName(), "\n"
          "given platform specification with empty\n"
          "  version=\n"
          "field.\n"
          ));
      /* clang-format on */
      return false;
    }

    /* clang-format off */
    mf->IssueMessage(MessageType::FATAL_ERROR, cmStrCat(
        "Generator\n"
        "  ", this->GetName(), "\n"
        "given platform specification containing a\n"
        "  version=", version, "\n"
        "field with unsupported value.\n"
        ));
    /* clang-format on */
    return false;
  }

  // If we are targeting Windows 10+, we select a Windows 10 SDK.
  // If no Windows 8.1 SDK is installed, which is possible with VS 2017 and
  // higher, then we must choose a Windows 10 SDK anyway.
  if (cmHasLiteralPrefix(this->SystemVersion, "10.0") ||
      !this->IsWin81SDKInstalled()) {
    return this->SelectWindows10SDK(mf);
  }

  // Under CMP0149 NEW behavior, we search for a Windows 10 SDK even
  // when targeting older Windows versions, but it is not required.
  if (mf->GetPolicyStatus(cmPolicies::CMP0149) == cmPolicies::NEW) {
    std::string const version = this->GetWindows10SDKVersion(mf);
    if (!version.empty()) {
      this->SetWindowsTargetPlatformVersion(version, mf);
      return true;
    }
  }

  // We are not targeting Windows 10+, so fall back to the Windows 8.1 SDK.
  // For VS 2019 and above we must explicitly specify it.
  if (this->Version >= cmGlobalVisualStudioGenerator::VSVersion::VS16 &&
      !cmSystemTools::VersionCompareGreater(this->SystemVersion, "8.1")) {
    this->SetWindowsTargetPlatformVersion("8.1", mf);
  }
  return true;
}

bool cmGlobalVisualStudio14Generator::VerifyNoGeneratorPlatformVersion(
  cmMakefile* mf) const
{
  if (!this->GeneratorPlatformVersion) {
    return true;
  }
  std::ostringstream e;
  /* clang-format off */
  e <<
    "Generator\n"
    "  " << this->GetName() << "\n"
    "given platform specification containing a\n"
    "  version=" << *this->GeneratorPlatformVersion << "\n"
    "field.  The version field is not supported when targeting\n"
    "  " << this->SystemName << ' ' << this->SystemVersion << '\n'
    ;
  /* clang-format on */
  mf->IssueMessage(MessageType::FATAL_ERROR, e.str());
  return false;
}

bool cmGlobalVisualStudio14Generator::InitializeWindowsStore(cmMakefile* mf)
{
  if (!this->SelectWindowsStoreToolset(this->DefaultPlatformToolset)) {
    std::string e;
    if (this->DefaultPlatformToolset.empty()) {
      e = cmStrCat(this->GetName(),
                   " supports Windows Store '8.0', '8.1' and "
                   "'10.0', but not '",
                   this->SystemVersion, "'.  Check CMAKE_SYSTEM_VERSION.");
    } else {
      e = cmStrCat(
        "A Windows Store component with CMake requires both the Windows "
        "Desktop SDK as well as the Windows Store '",
        this->SystemVersion,
        "' SDK. Please make sure that you have both installed");
    }
    mf->IssueMessage(MessageType::FATAL_ERROR, e);
    return false;
  }
  return true;
}

bool cmGlobalVisualStudio14Generator::InitializeAndroid(cmMakefile*)
{
  return true;
}

bool cmGlobalVisualStudio14Generator::ProcessGeneratorPlatformField(
  std::string const& key, std::string const& value)
{
  if (key == "version"_s) {
    this->GeneratorPlatformVersion = value;
    return true;
  }
  return false;
}

bool cmGlobalVisualStudio14Generator::SelectWindows10SDK(cmMakefile* mf)
{
  // Find the default version of the Windows 10 SDK.
  std::string const version = this->GetWindows10SDKVersion(mf);

  if (version.empty()) {
    if (this->GeneratorPlatformVersion) {
      mf->IssueMessage(
        MessageType::FATAL_ERROR,
        cmStrCat("Generator\n  ", this->GetName(),
                 "\ngiven platform specification with\n  version=",
                 *this->GeneratorPlatformVersion,
                 "\nfield, but no Windows SDK with that version was found."));
      return false;
    }

    if (this->SystemName == "WindowsStore"_s) {
      mf->IssueMessage(
        MessageType::FATAL_ERROR,
        "Could not find an appropriate version of the Windows 10 SDK"
        " installed on this machine");
      return false;
    }
  }

  this->SetWindowsTargetPlatformVersion(version, mf);
  return true;
}

void cmGlobalVisualStudio14Generator::SetWindowsTargetPlatformVersion(
  std::string const& version, cmMakefile* mf)
{
  this->WindowsTargetPlatformVersion = version;
  if (!this->WindowsTargetPlatformVersion.empty() &&
      !cmSystemTools::VersionCompareEqual(this->WindowsTargetPlatformVersion,
                                          this->SystemVersion)) {
    mf->DisplayStatus(cmStrCat("Selecting Windows SDK version ",
                               this->WindowsTargetPlatformVersion,
                               " to target Windows ", this->SystemVersion,
                               '.'),
                      -1);
  }
  mf->AddDefinition("CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION",
                    this->WindowsTargetPlatformVersion);
}

bool cmGlobalVisualStudio14Generator::SelectWindowsStoreToolset(
  std::string& toolset) const
{
  if (cmHasLiteralPrefix(this->SystemVersion, "10.0")) {
    if (this->IsWindowsStoreToolsetInstalled() &&
        this->IsWindowsDesktopToolsetInstalled()) {
      toolset = "v140";
      return true;
    }
    return false;
  }
  return this->cmGlobalVisualStudio12Generator::SelectWindowsStoreToolset(
    toolset);
}

bool cmGlobalVisualStudio14Generator::IsWindowsDesktopToolsetInstalled() const
{
  char const desktop10Key[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
                              "VisualStudio\\14.0\\VC\\Runtimes";

  std::vector<std::string> vc14;
  return cmSystemTools::GetRegistrySubKeys(desktop10Key, vc14,
                                           cmSystemTools::KeyWOW64_32);
}

bool cmGlobalVisualStudio14Generator::IsWindowsStoreToolsetInstalled() const
{
  char const universal10Key[] =
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
    "VisualStudio\\14.0\\Setup\\Build Tools for Windows 10;SrcPath";

  std::string win10SDK;
  return cmSystemTools::ReadRegistryValue(universal10Key, win10SDK,
                                          cmSystemTools::KeyWOW64_32);
}

bool cmGlobalVisualStudio14Generator::IsWin81SDKInstalled() const
{
  return true;
}

std::string cmGlobalVisualStudio14Generator::GetWindows10SDKMaxVersion(
  cmMakefile* mf) const
{
  // if the given value is set, it can either be OFF/FALSE or a valid SDK
  // string
  if (cmValue value = mf->GetDefinition(
        "CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM")) {

    // If the value is some off/false value, then there is NO maximum set.
    if (value.IsOff()) {
      return std::string();
    }
    // If the value is something else, trust that it is a valid SDK value.
    if (value) {
      return *value;
    }
    // If value is an invalid pointer, leave result unchanged.
  }

  return this->GetWindows10SDKMaxVersionDefault(mf);
}

std::string cmGlobalVisualStudio14Generator::GetWindows10SDKMaxVersionDefault(
  cmMakefile*) const
{
  // The last Windows 10 SDK version that VS 2015 can target is 10.0.14393.0.
  //
  // "VS 2015 Users: The Windows 10 SDK (15063, 16299, 17134, 17763) is
  // officially only supported for VS 2017." From:
  // https://blogs.msdn.microsoft.com/chuckw/2018/10/02/windows-10-october-2018-update/
  return "10.0.14393.0";
}

#if defined(_WIN32) && !defined(__CYGWIN__)
struct NoWindowsH
{
  bool operator()(std::string const& p)
  {
    return !cmSystemTools::FileExists(cmStrCat(p, "/um/windows.h"), true);
  }
};
class WindowsSDKTooRecent
{
  std::string const& MaxVersion;

public:
  WindowsSDKTooRecent(std::string const& maxVersion)
    : MaxVersion(maxVersion)
  {
  }
  bool operator()(std::string const& v)
  {
    return cmSystemTools::VersionCompareGreater(v, MaxVersion);
  }
};
#endif

std::string cmGlobalVisualStudio14Generator::GetWindows10SDKVersion(
  cmMakefile* mf)
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  std::vector<std::string> win10Roots;

  {
    std::string win10Root;
    if (cmSystemTools::GetEnv("CMAKE_WINDOWS_KITS_10_DIR", win10Root)) {
      cmSystemTools::ConvertToUnixSlashes(win10Root);
      win10Roots.push_back(win10Root);
    }
  }

  {
    // This logic is taken from the vcvarsqueryregistry.bat file from VS2015
    // Try HKLM and then HKCU.
    std::string win10Root;
    if (cmSystemTools::ReadRegistryValue(
          "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
          "Windows Kits\\Installed Roots;KitsRoot10",
          win10Root, cmSystemTools::KeyWOW64_32) ||
        cmSystemTools::ReadRegistryValue(
          "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\"
          "Windows Kits\\Installed Roots;KitsRoot10",
          win10Root, cmSystemTools::KeyWOW64_32)) {
      cmSystemTools::ConvertToUnixSlashes(win10Root);
      win10Roots.push_back(win10Root);
    }
  }

  if (win10Roots.empty()) {
    return std::string();
  }

  std::vector<std::string> sdks;
  // Grab the paths of the different SDKs that are installed
  for (std::string const& i : win10Roots) {
    std::string path = cmStrCat(i, "/Include/*");
    cmSystemTools::GlobDirs(path, sdks);
  }

  // Skip SDKs that do not contain <um/windows.h> because that indicates that
  // only the UCRT MSIs were installed for them.
  cm::erase_if(sdks, NoWindowsH());

  // Only use the filename, which will be the SDK version.
  for (std::string& i : sdks) {
    i = cmSystemTools::GetFilenameName(i);
  }

  // Skip SDKs that cannot be used with our toolset, unless the user does not
  // want to limit the highest supported SDK according to the Microsoft
  // documentation.
  std::string maxVersion = this->GetWindows10SDKMaxVersion(mf);
  if (!maxVersion.empty()) {
    cm::erase_if(sdks, WindowsSDKTooRecent(maxVersion));
  }

  // Sort the results to make sure we select the most recent one.
  std::sort(sdks.begin(), sdks.end(), cmSystemTools::VersionCompareGreater);

  // Look for a SDK exactly matching the requested version, if any.
  if (this->GeneratorPlatformVersion) {
    for (std::string const& i : sdks) {
      if (cmSystemTools::VersionCompareEqual(
            i, *this->GeneratorPlatformVersion)) {
        return i;
      }
    }
    // An exact version was requested but not found.
    // Our caller will issue the error message.
    return std::string();
  }

  if (mf->GetPolicyStatus(cmPolicies::CMP0149) == cmPolicies::NEW) {
    if (cm::optional<std::string> const envVer =
          cmSystemTools::GetEnvVar("WindowsSDKVersion")) {
      // Look for a SDK exactly matching the environment variable.
      for (std::string const& i : sdks) {
        if (cmSystemTools::VersionCompareEqual(i, *envVer)) {
          return i;
        }
      }
    }
  } else {
    // Look for a SDK exactly matching the target Windows version.
    for (std::string const& i : sdks) {
      if (cmSystemTools::VersionCompareEqual(i, this->SystemVersion)) {
        return i;
      }
    }
  }

  if (!sdks.empty()) {
    // Use the latest Windows 10 SDK since the exact version is not available.
    return sdks.at(0);
  }
#endif
  (void)mf;
  // Return an empty string
  return std::string();
}

void cmGlobalVisualStudio14Generator::AddSolutionItems(cmLocalGenerator* root)
{
  cmValue n = root->GetMakefile()->GetProperty("VS_SOLUTION_ITEMS");
  if (cmNonempty(n)) {
    cmMakefile* makefile = root->GetMakefile();

    std::vector<cmSourceGroup> sourceGroups = makefile->GetSourceGroups();

    cmVisualStudioFolder* defaultFolder = nullptr;

    std::vector<std::string> pathComponents = {
      makefile->GetCurrentSourceDirectory(),
      "",
      "",
    };

    for (std::string const& relativePath : cmList(n)) {
      pathComponents[2] = relativePath;

      std::string fullPath = cmSystemTools::FileIsFullPath(relativePath)
        ? relativePath
        : cmSystemTools::JoinPath(pathComponents);

      cmSourceGroup* sg = makefile->FindSourceGroup(fullPath, sourceGroups);

      cmVisualStudioFolder* folder = nullptr;
      if (!sg->GetFullName().empty()) {
        std::string folderPath = sg->GetFullName();
        // Source groups use '\' while solution folders use '/'.
        cmSystemTools::ReplaceString(folderPath, "\\", "/");
        folder = this->CreateSolutionFolders(folderPath);
      } else {
        // Lazily initialize the default solution items folder.
        if (defaultFolder == nullptr) {
          defaultFolder = this->CreateSolutionFolders("Solution Items");
        }
        folder = defaultFolder;
      }

      folder->SolutionItems.insert(fullPath);
    }
  }
}

void cmGlobalVisualStudio14Generator::WriteFolderSolutionItems(
  std::ostream& fout, cmVisualStudioFolder const& folder)
{
  fout << "\tProjectSection(SolutionItems) = preProject\n";

  for (std::string const& item : folder.SolutionItems) {
    fout << "\t\t" << item << " = " << item << "\n";
  }

  fout << "\tEndProjectSection\n";
}

#endif //#ifdef __MINGW32__