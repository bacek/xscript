#ifndef _XSCRIPT_LOCAL_ARG_LIST_H_
#define _XSCRIPT_LOCAL_ARG_LIST_H_

#include "xscript/args.h"

#include "xscript/typed_map.h"

namespace xscript {

class LocalArgList : public CommonArgList {
public:
    LocalArgList();
    virtual ~LocalArgList();
    virtual void addAs(const std::string &type, const TypedValue &value);
};

} // namespace xscript

#endif // _XSCRIPT_LOCAL_ARG_LIST_H_
