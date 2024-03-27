#ifndef CFGCommon_H
#define CFGCommon_H

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#define __FILENAME__ \
  (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define CFG_PRINT_MINIMUM_SIZE (256)
#define CFG_PRINT_MAXIMUM_SIZE (8192)
typedef std::chrono::high_resolution_clock::time_point CFG_TIME;

typedef void (*cfg_callback_post_msg_function)(const std::string& message,
                                               const bool raw);
typedef void (*cfg_callback_post_err_function)(const std::string& message,
                                               bool append);
typedef int (*cfg_callback_execute_command)(const std::string& command,
                                            const std::string logFile,
                                            bool appendLog);

class CFGArg;
struct CFGCommon_ARG {
  std::string command;
  std::string device;
  std::string projectName;
  std::string projectPath;
  std::string taskPath;
  std::string analyzePath;
  std::string synthesisPath;
  std::string binPath;
  std::string dataPath;
  std::string compilerName;
  std::filesystem::path toolPath;    // for any tool path
  std::filesystem::path searchPath;  // for any search path
  bool clean;
  std::string tclOutput;
  int tclStatus = 0;  // TCL_OK or TCL_ERROR
  std::shared_ptr<CFGArg> arg;
  std::vector<std::string> raws;
};

std::string CFG_print(const char* format_string, ...);

void CFG_assertion(const char* file, const char* func, size_t line,
                   std::string msg);

#define CFG_INTERNAL_ERROR(...) \
  { CFG_assertion(__FILE__, __func__, __LINE__, CFG_print(__VA_ARGS__)); }

#if defined(_MSC_VER)

#define CFG_ASSERT(truth)                                \
  if (!(truth)) {                                        \
    CFG_assertion(__FILE__, __func__, __LINE__, #truth); \
  }

#define CFG_ASSERT_MSG(truth, ...)                                       \
  if (!(truth)) {                                                        \
    CFG_assertion(__FILE__, __func__, __LINE__, CFG_print(__VA_ARGS__)); \
  }

#else

#define CFG_ASSERT(truth)                                \
  if (!(__builtin_expect(!!(truth), 0))) {               \
    CFG_assertion(__FILE__, __func__, __LINE__, #truth); \
  }

#define CFG_ASSERT_MSG(truth, ...)                                       \
  if (!(__builtin_expect(!!(truth), 0))) {                               \
    CFG_assertion(__FILE__, __func__, __LINE__, CFG_print(__VA_ARGS__)); \
  }

#endif

std::string CFG_get_time();

CFG_TIME CFG_time_begin();

uint64_t CFG_nano_time_elapse(CFG_TIME begin);

float CFG_time_elapse(CFG_TIME begin);

void CFG_set_callback_message_function(cfg_callback_post_msg_function msg,
                                       cfg_callback_post_err_function err,
                                       cfg_callback_execute_command exec);
void CFG_set_callback_post_msg_function(cfg_callback_post_msg_function msg);
void CFG_set_callback_post_err_function(cfg_callback_post_err_function err);
void CFG_set_callback_exec_cmd_function(cfg_callback_execute_command exec);

void CFG_unset_callback_message_function();
void CFG_unset_callback_post_msg_function();
void CFG_unset_callback_post_err_function();
void CFG_unset_callback_exec_cmd_function();

void CFG_post_msg(const std::string& message,
                  const std::string pre_msg = "INFO: ",
                  const bool new_line = true);

void CFG_post_warning(const std::string& message);

void CFG_post_err(const std::string& message, bool append);

int CFG_execute_and_monitor_system_command(
    const std::string& command, const std::string logFile = std::string{},
    bool appendLog = false);

std::string CFG_replace_string(std::string string, const std::string& original,
                               const std::string& replacement,
                               bool no_double_replacment = true);

std::string CFG_change_directory_to_linux_format(std::string path);

std::string CFG_get_configuration_relative_path(std::string path);

void CFG_get_rid_trailing_whitespace(std::string& string,
                                     const std::vector<char> whitespaces = {
                                         ' ', '\t', '\n', '\r'});

void CFG_get_rid_leading_whitespace(std::string& string,
                                    const std::vector<char> whitespaces = {
                                        ' ', '\t', '\n', '\r'});

void CFG_get_rid_whitespace(std::string& string,
                            const std::vector<char> whitespaces = {' ', '\t',
                                                                   '\n', '\r'});
std::string CFG_string_toupper(std::string& string);

std::string CFG_string_tolower(std::string& string);

uint64_t CFG_convert_string_to_u64(std::string string, bool no_empty = false,
                                   bool* status = nullptr,
                                   const uint64_t* const init_value = nullptr);

std::string CFG_convert_number_to_unit_string(uint64_t number);

int CFG_find_string_in_vector(const std::vector<std::string>& vector,
                              const std::string element);

int CFG_find_u32_in_vector(const std::vector<uint32_t>& vector,
                           const uint32_t element);

std::vector<std::string> CFG_split_string(const std::string& str,
                                          const std::string& seperator,
                                          int max_split = 0,
                                          bool include_empty = true);

int CFG_compiler_execute_cmd(const std::string& command,
                             const std::string logFile = std::string{},
                             bool appendLog = false);

int CFG_execute_cmd(const std::string& cmd, std::string& output,
                    std::ostream* outStream, std::atomic<bool>& stopCommand);

int CFG_execute_cmd_with_callback(
    const std::string& cmd, std::string& output, std::ostream* outstream,
    std::regex patternToMatch, std::atomic<bool>& stopCommand,
    std::function<void(const std::string&)> progressCallback = nullptr,
    std::function<void(const std::string&)> generalCallback = nullptr);

std::filesystem::path CFG_find_file(const std::filesystem::path& filePath,
                                    const std::filesystem::path& defaultDir);

void CFG_sleep_ms(uint32_t milisecond);

void CFG_read_text_file(const std::string& filepath,
                        std::vector<std::string>& data,
                        bool trim_trailer_whitespace);

void CFG_read_binary_file(const std::string& filepath,
                          std::vector<uint8_t>& data);

void CFG_write_binary_file(const std::string& filepath, const uint8_t* data,
                           const size_t data_size);

bool CFG_compare_two_text_files(const std::string& filepath1,
                                const std::string& filepath2,
                                bool debug_if_diff = false);

bool CFG_compare_two_binary_files(const std::string& filepath1,
                                  const std::string& filepath2);

void CFG_Python(std::vector<std::string> commands,
                std::vector<std::string> strs, std::vector<std::string> ints,
                std::map<std::string, std::string>& str_maps,
                std::map<std::string, uint32_t>& int_maps);

#define CFG_POST_MSG(...) \
  { CFG_post_msg(CFG_print(__VA_ARGS__)); }

#define CFG_POST_WARNING(...) \
  { CFG_post_warning(CFG_print(__VA_ARGS__)); }

#define CFG_POST_ERR(...) \
  { CFG_post_err(CFG_print(__VA_ARGS__), true); }

#define CFG_POST_ERR_NO_APPEND(...) \
  { CFG_post_err(CFG_print(__VA_ARGS__), false); }

// Use the normal message for now
#define CFG_POST_DBG(...) \
  { CFG_post_msg(CFG_print(__VA_ARGS__)); }

#endif
