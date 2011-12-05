#include "settings.h"

#include "xscript/request_file.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

RequestFile::RequestFile(const Range &remote_name, const Range &type, const Range &content) :
    data_(content.begin(), static_cast<std::streamsize>(content.size()))
{
    if (!remote_name.empty()) {
        remote_name_.assign(remote_name.begin(), remote_name.end());
    }
    if (!type.empty()) {
        type_.assign(type.begin(), type.end());
    }
}

RequestFile::~RequestFile() {
}
	
} // namespace xscript
