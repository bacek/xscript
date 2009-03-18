#include <string>

namespace xscript {
namespace offline {

void initialize(const char *config_path);

std::string
renderBuffer(const std::string &url,
             const std::string &xml,
             const std::string &body,
             const std::string &headers,
             const std::string &vars);

std::string
renderFile(const std::string &file,
           const std::string &body,
           const std::string &headers,
           const std::string &vars);

}}
