#ifndef _XSCRIPT_REQUEST_FILE_H_
#define _XSCRIPT_REQUEST_FILE_H_

#include <string>
#include <utility>
#include <vector>

#include "xscript/range.h"

namespace xscript {

class RequestFile {
public:
    RequestFile(const Range &remote_name, const Range &type, const Range &content);
    virtual ~RequestFile();

    const std::string& remoteName() const { return remote_name_; }
    const std::string& type() const { return type_; }

    std::pair<const char*, std::streamsize> data() const { return data_; }

private:
    std::string remote_name_, type_;
    std::pair<const char*, std::streamsize> data_;
};

typedef std::vector<RequestFile> RequestFiles;

} // namespace xscript

#endif // _XSCRIPT_REQUEST_FILE_H_
