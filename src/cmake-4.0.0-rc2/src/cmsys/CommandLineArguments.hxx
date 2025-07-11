/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_CommandLineArguments_hxx
#define cmsys_CommandLineArguments_hxx

#include <cmsys/Configure.h>
#include <cmsys/Configure.hxx>

#include <string>
#include <vector>

namespace cmsys {

class CommandLineArgumentsInternal;
struct CommandLineArgumentsCallbackStructure;

/** \class CommandLineArguments
 * \brief Command line arguments processing code.
 *
 * Find specified arguments with optional options and execute specified methods
 * or set given variables.
 *
 * The two interfaces it knows are callback based and variable based. For
 * callback based, you have to register callback for particular argument using
 * AddCallback method. When that argument is passed, the callback will be
 * called with argument, value, and call data. For boolean (NO_ARGUMENT)
 * arguments, the value is "1". If the callback returns 0 the argument parsing
 * will stop with an error.
 *
 * For the variable interface you associate variable with each argument. When
 * the argument is specified, the variable is set to the specified value casted
 * to the appropriate type. For boolean (NO_ARGUMENT), the value is "1".
 *
 * Both interfaces can be used at the same time.
 *
 * Possible argument types are:
 *   NO_ARGUMENT     - The argument takes no value             : --A
 *   CONCAT_ARGUMENT - The argument takes value after no space : --Aval
 *   SPACE_ARGUMENT  - The argument takes value after space    : --A val
 *   EQUAL_ARGUMENT  - The argument takes value after equal    : --A=val
 *   MULTI_ARGUMENT  - The argument takes values after space   : --A val1 val2
 * val3 ...
 *
 * Example use:
 *
 * kwsys::CommandLineArguments arg;
 * arg.Initialize(argc, argv);
 * typedef kwsys::CommandLineArguments argT;
 * arg.AddArgument("--something", argT::EQUAL_ARGUMENT, &some_variable,
 *                 "This is help string for --something");
 * if ( !arg.Parse() )
 *   {
 *   std::cerr << "Problem parsing arguments" << std::endl;
 *   res = 1;
 *   }
 *
 */

class cmsys_EXPORT CommandLineArguments
{
public:
  CommandLineArguments();
  ~CommandLineArguments();

  CommandLineArguments(CommandLineArguments const&) = delete;
  CommandLineArguments& operator=(CommandLineArguments const&) = delete;

  /**
   * Various argument types.
   */
  enum ArgumentTypeEnum
  {
    NO_ARGUMENT,
    CONCAT_ARGUMENT,
    SPACE_ARGUMENT,
    EQUAL_ARGUMENT,
    MULTI_ARGUMENT
  };

  /**
   * Various variable types. When using the variable interface, this specifies
   * what type the variable is.
   */
  enum VariableTypeEnum
  {
    NO_VARIABLE_TYPE = 0,   // The variable is not specified
    INT_TYPE,               // The variable is integer (int)
    BOOL_TYPE,              // The variable is boolean (bool)
    DOUBLE_TYPE,            // The variable is float (double)
    STRING_TYPE,            // The variable is string (char*)
    STL_STRING_TYPE,        // The variable is string (char*)
    VECTOR_INT_TYPE,        // The variable is integer (int)
    VECTOR_BOOL_TYPE,       // The variable is boolean (bool)
    VECTOR_DOUBLE_TYPE,     // The variable is float (double)
    VECTOR_STRING_TYPE,     // The variable is string (char*)
    VECTOR_STL_STRING_TYPE, // The variable is string (char*)
    LAST_VARIABLE_TYPE
  };

  /**
   * Prototypes for callbacks for callback interface.
   */
  typedef int (*CallbackType)(char const* argument, char const* value,
                              void* call_data);
  typedef int (*ErrorCallbackType)(char const* argument, void* client_data);

  /**
   * Initialize internal data structures. This should be called before parsing.
   */
  void Initialize(int argc, char const* const argv[]);
  void Initialize(int argc, char* argv[]);

  /**
   * Initialize internal data structure and pass arguments one by one. This is
   * convenience method for use from scripting languages where argc and argv
   * are not available.
   */
  void Initialize();
  void ProcessArgument(char const* arg);

  /**
   * This method will parse arguments and call appropriate methods.
   */
  int Parse();

  /**
   * This method will add a callback for a specific argument. The arguments to
   * it are argument, argument type, callback method, and call data. The
   * argument help specifies the help string used with this option. The
   * callback and call_data can be skipped.
   */
  void AddCallback(char const* argument, ArgumentTypeEnum type,
                   CallbackType callback, void* call_data, char const* help);

  /**
   * Add handler for argument which is going to set the variable to the
   * specified value. If the argument is specified, the option is casted to the
   * appropriate type.
   */
  void AddArgument(char const* argument, ArgumentTypeEnum type, bool* variable,
                   char const* help);
  void AddArgument(char const* argument, ArgumentTypeEnum type, int* variable,
                   char const* help);
  void AddArgument(char const* argument, ArgumentTypeEnum type,
                   double* variable, char const* help);
  void AddArgument(char const* argument, ArgumentTypeEnum type,
                   char** variable, char const* help);
  void AddArgument(char const* argument, ArgumentTypeEnum type,
                   std::string* variable, char const* help);

  /**
   * Add handler for argument which is going to set the variable to the
   * specified value. If the argument is specified, the option is casted to the
   * appropriate type. This will handle the multi argument values.
   */
  void AddArgument(char const* argument, ArgumentTypeEnum type,
                   std::vector<bool>* variable, char const* help);
  void AddArgument(char const* argument, ArgumentTypeEnum type,
                   std::vector<int>* variable, char const* help);
  void AddArgument(char const* argument, ArgumentTypeEnum type,
                   std::vector<double>* variable, char const* help);
  void AddArgument(char const* argument, ArgumentTypeEnum type,
                   std::vector<char*>* variable, char const* help);
  void AddArgument(char const* argument, ArgumentTypeEnum type,
                   std::vector<std::string>* variable, char const* help);

  /**
   * Add handler for boolean argument. The argument does not take any option
   * and if it is specified, the value of the variable is true/1, otherwise it
   * is false/0.
   */
  void AddBooleanArgument(char const* argument, bool* variable,
                          char const* help);
  void AddBooleanArgument(char const* argument, int* variable,
                          char const* help);
  void AddBooleanArgument(char const* argument, double* variable,
                          char const* help);
  void AddBooleanArgument(char const* argument, char** variable,
                          char const* help);
  void AddBooleanArgument(char const* argument, std::string* variable,
                          char const* help);

  /**
   * Set the callbacks for error handling.
   */
  void SetClientData(void* client_data);
  void SetUnknownArgumentCallback(ErrorCallbackType callback);

  /**
   * Get remaining arguments. It allocates space for argv, so you have to call
   * delete[] on it.
   */
  void GetRemainingArguments(int* argc, char*** argv);
  void DeleteRemainingArguments(int argc, char*** argv);

  /**
   * If StoreUnusedArguments is set to true, then all unknown arguments will be
   * stored and the user can access the modified argc, argv without known
   * arguments.
   */
  void StoreUnusedArguments(bool val) { this->StoreUnusedArgumentsFlag = val; }
  void GetUnusedArguments(int* argc, char*** argv);

  /**
   * Return string containing help. If the argument is specified, only return
   * help for that argument.
   */
  char const* GetHelp() { return this->Help.c_str(); }
  char const* GetHelp(char const* arg);

  /**
   * Get / Set the help line length. This length is used when generating the
   * help page. Default length is 80.
   */
  void SetLineLength(unsigned int);
  unsigned int GetLineLength();

  /**
   * Get the executable name (argv0). This is only available when using
   * Initialize with argc/argv.
   */
  char const* GetArgv0();

  /**
   * Get index of the last argument parsed. This is the last argument that was
   * parsed ok in the original argc/argv list.
   */
  unsigned int GetLastArgument();

protected:
  void GenerateHelp();

  //! This is internal method that registers variable with argument
  void AddArgument(char const* argument, ArgumentTypeEnum type,
                   VariableTypeEnum vtype, void* variable, char const* help);

  bool GetMatchedArguments(std::vector<std::string>* matches,
                           std::string const& arg);

  //! Populate individual variables
  bool PopulateVariable(CommandLineArgumentsCallbackStructure* cs,
                        char const* value);

  //! Populate individual variables of type ...
  void PopulateVariable(bool* variable, std::string const& value);
  void PopulateVariable(int* variable, std::string const& value);
  void PopulateVariable(double* variable, std::string const& value);
  void PopulateVariable(char** variable, std::string const& value);
  void PopulateVariable(std::string* variable, std::string const& value);
  void PopulateVariable(std::vector<bool>* variable, std::string const& value);
  void PopulateVariable(std::vector<int>* variable, std::string const& value);
  void PopulateVariable(std::vector<double>* variable,
                        std::string const& value);
  void PopulateVariable(std::vector<char*>* variable,
                        std::string const& value);
  void PopulateVariable(std::vector<std::string>* variable,
                        std::string const& value);

  typedef CommandLineArgumentsInternal Internal;
  Internal* Internals;
  std::string Help;

  unsigned int LineLength;

  bool StoreUnusedArgumentsFlag;
};

} // namespace cmsys

#endif
