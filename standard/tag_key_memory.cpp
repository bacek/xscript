#include <boost/filesystem.hpp>

#include "xscript/param.h"
#include "xscript/tagged_block.h"
#include "tag_key_memory.h"

namespace xscript {

TagKeyMemory::TagKeyMemory(const Context *ctx, const TaggedBlock *block) : value_() {
    assert(NULL != ctx);
    assert(NULL != block);

    const std::string& xslt = block->xsltName();
    if (!xslt.empty()) {
        value_.assign(xslt);
        value_.push_back('|');
        
        namespace fs = boost::filesystem;
        fs::path path(xslt);
        if (fs::exists(path) && !fs::is_directory(path)) {
            value_.append(boost::lexical_cast<std::string>(fs::last_write_time(path)));
            value_.push_back('|');
        }
        else {
            throw std::runtime_error("Cannot stat stylesheet " + xslt);
        }
    }
    value_.append(block->canonicalMethod(ctx));

    const std::vector<Param*> &params = block->params();
    for (unsigned int i = 0, n = params.size(); i < n; ++i) {
        value_.append(1, ':').append(params[i]->asString(ctx));
    }

    const std::vector<Param*> &xslt_params = block->xsltParams();
    for (unsigned int i = 0, n = xslt_params.size(); i < n; ++i) {
        value_.append(1, ':').append(xslt_params[i]->id());
        value_.append(1, ':').append(xslt_params[i]->asString(ctx));
    }
}

const std::string&
TagKeyMemory::asString() const {
    return value_;
}

}
