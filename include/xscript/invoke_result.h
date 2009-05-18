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

    XmlDocHelper doc;
    Type type;
};

} // namespace xscript

#endif // _XSCRIPT_INVOKE_RESULT_H_

