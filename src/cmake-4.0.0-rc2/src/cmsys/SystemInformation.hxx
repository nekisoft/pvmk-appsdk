/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_SystemInformation_h
#define cmsys_SystemInformation_h

#include <cmsys/Configure.hxx>

#include <stddef.h> /* size_t */
#include <string>

namespace cmsys {

// forward declare the implementation class
class SystemInformationImplementation;

class cmsys_EXPORT SystemInformation
{
  friend class SystemInformationImplementation;
  SystemInformationImplementation* Implementation;

public:
  // possible parameter values for DoesCPUSupportFeature()
  static long int const CPU_FEATURE_MMX = 1 << 0;
  static long int const CPU_FEATURE_MMX_PLUS = 1 << 1;
  static long int const CPU_FEATURE_SSE = 1 << 2;
  static long int const CPU_FEATURE_SSE2 = 1 << 3;
  static long int const CPU_FEATURE_AMD_3DNOW = 1 << 4;
  static long int const CPU_FEATURE_AMD_3DNOW_PLUS = 1 << 5;
  static long int const CPU_FEATURE_IA64 = 1 << 6;
  static long int const CPU_FEATURE_MP_CAPABLE = 1 << 7;
  static long int const CPU_FEATURE_HYPERTHREAD = 1 << 8;
  static long int const CPU_FEATURE_SERIALNUMBER = 1 << 9;
  static long int const CPU_FEATURE_APIC = 1 << 10;
  static long int const CPU_FEATURE_SSE_FP = 1 << 11;
  static long int const CPU_FEATURE_SSE_MMX = 1 << 12;
  static long int const CPU_FEATURE_CMOV = 1 << 13;
  static long int const CPU_FEATURE_MTRR = 1 << 14;
  static long int const CPU_FEATURE_L1CACHE = 1 << 15;
  static long int const CPU_FEATURE_L2CACHE = 1 << 16;
  static long int const CPU_FEATURE_L3CACHE = 1 << 17;
  static long int const CPU_FEATURE_ACPI = 1 << 18;
  static long int const CPU_FEATURE_THERMALMONITOR = 1 << 19;
  static long int const CPU_FEATURE_TEMPSENSEDIODE = 1 << 20;
  static long int const CPU_FEATURE_FREQUENCYID = 1 << 21;
  static long int const CPU_FEATURE_VOLTAGEID_FREQUENCY = 1 << 22;
  static long int const CPU_FEATURE_FPU = 1 << 23;

public:
  SystemInformation();
  ~SystemInformation();

  SystemInformation(SystemInformation const&) = delete;
  SystemInformation& operator=(SystemInformation const&) = delete;

  char const* GetVendorString();
  char const* GetVendorID();
  std::string GetTypeID();
  std::string GetFamilyID();
  std::string GetModelID();
  std::string GetModelName();
  std::string GetSteppingCode();
  char const* GetExtendedProcessorName();
  char const* GetProcessorSerialNumber();
  int GetProcessorCacheSize();
  unsigned int GetLogicalProcessorsPerPhysical();
  float GetProcessorClockFrequency();
  int GetProcessorAPICID();
  int GetProcessorCacheXSize(long int);
  bool DoesCPUSupportFeature(long int);

  // returns an informative general description of the cpu
  // on this system.
  std::string GetCPUDescription();

  char const* GetHostname();
  std::string GetFullyQualifiedDomainName();

  char const* GetOSName();
  char const* GetOSRelease();
  char const* GetOSVersion();
  char const* GetOSPlatform();

  int GetOSIsWindows();
  int GetOSIsLinux();
  int GetOSIsApple();

  // returns an informative general description of the os
  // on this system.
  std::string GetOSDescription();

  // returns if the operating system is 64bit or not.
  bool Is64Bits();

  unsigned int GetNumberOfLogicalCPU();
  unsigned int GetNumberOfPhysicalCPU();

  bool DoesCPUSupportCPUID();

  // Retrieve id of the current running process
  long long GetProcessId();

  // Retrieve memory information in MiB.
  size_t GetTotalVirtualMemory();
  size_t GetAvailableVirtualMemory();
  size_t GetTotalPhysicalMemory();
  size_t GetAvailablePhysicalMemory();

  // returns an informative general description if the installed and
  // available ram on this system. See the GetHostMemoryTotal, and
  // Get{Host,Proc}MemoryAvailable methods for more information.
  std::string GetMemoryDescription(char const* hostLimitEnvVarName = nullptr,
                                   char const* procLimitEnvVarName = nullptr);

  // Retrieve amount of physical memory installed on the system in KiB
  // units.
  long long GetHostMemoryTotal();

  // Get total system RAM in units of KiB available colectivley to all
  // processes in a process group. An example of a process group
  // are the processes comprising an mpi program which is running in
  // parallel. The amount of memory reported may differ from the host
  // total if a host wide resource limit is applied. Such reource limits
  // are reported to us via an application specified environment variable.
  long long GetHostMemoryAvailable(char const* hostLimitEnvVarName = nullptr);

  // Get total system RAM in units of KiB available to this process.
  // This may differ from the host available if a per-process resource
  // limit is applied. per-process memory limits are applied on unix
  // system via rlimit API. Resource limits that are not imposed via
  // rlimit API may be reported to us via an application specified
  // environment variable.
  long long GetProcMemoryAvailable(char const* hostLimitEnvVarName = nullptr,
                                   char const* procLimitEnvVarName = nullptr);

  // Get the system RAM used by all processes on the host, in units of KiB.
  long long GetHostMemoryUsed();

  // Get system RAM used by this process id in units of KiB.
  long long GetProcMemoryUsed();

  // Return the load average of the machine or -0.0 if it cannot
  // be determined.
  double GetLoadAverage();

  // enable/disable stack trace signal handler. In order to
  // produce an informative stack trace the application should
  // be dynamically linked and compiled with debug symbols.
  static void SetStackTraceOnError(int enable);

  // format and return the current program stack in a string. In
  // order to produce an informative stack trace the application
  // should be dynamically linked and compiled with debug symbols.
  static std::string GetProgramStack(int firstFrame, int wholePath);

  /** Run the different checks */
  void RunCPUCheck();
  void RunOSCheck();
  void RunMemoryCheck();
};

} // namespace cmsys

#endif
