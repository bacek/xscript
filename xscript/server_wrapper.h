#include <string>

bool initialize(const char *path);

std::string
renderBuffer(const std::string &xml,
             const std::string &url,
             const std::string &docroot,
             const std::string &headers,
             const std::string &args);

std::string
renderFile(const std::string &file,
           const std::string &docroot,
           const std::string &headers,
           const std::string &args);
