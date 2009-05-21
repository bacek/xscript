#ifndef _XSCRIPT_INVOKE_RESULT_H_
#define _XSCRIPT_INVOKE_RESULT_H_

#include "xscript/xml_helpers.h"

namespace xscript {

struct InvokeResult {
    enum Type {
        ERROR  = 1,
        NO_CACHE = 2,
        SUCCESS  = 3,
    };
    
    InvokeResult() :
        doc(), type(ERROR) {
    }
    InvokeResult(XmlDocHelper document, Type result) :
        doc(document), type(result) {
    }

    bool success() const {
        return type == SUCCESS;
    }
    
    bool error() const {
        return type == ERROR;
    }
    
    bool nocache() const {
        return type == NO_CACHE;
    }
    
    XmlDocHelper doc;
    Type type;
};

} // namespace xscript

#endif // _XSCRIPT_INVOKE_RESULT_H_

