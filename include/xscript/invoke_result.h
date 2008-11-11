#ifndef _XSCRIPT_INVOKE_RESULT_H_
#define _XSCRIPT_INVOKE_RESULT_H_

#include "xscript/xml_helpers.h"

namespace xscript {

struct InvokeResult {
    InvokeResult() :
        doc(), success(false) {
    }
    InvokeResult(XmlDocHelper document, bool result) :
        doc(document), success(result) {
    }

    XmlDocHelper doc;
    bool success;
};

} // namespace xscript

#endif // _XSCRIPT_INVOKE_RESULT_H_

