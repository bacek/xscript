#ifndef _XSCRIPT_XSLT_PROFILER_H_
#define _XSCRIPT_XSLT_PROFILER_H_

#include <string>
#include <xscript/component.h>

#include <libxml/tree.h>

namespace xscript {

class Context;

class XsltProfiler : public Component<XsltProfiler> {
public:
    XsltProfiler();
    virtual ~XsltProfiler();

    virtual void insertProfileDoc(const std::string& name, xmlDocPtr doc);
    virtual void dumpProfileInfo(Context* ctx);
};

} // namespace xscript

#endif //_XSCRIPT_XSLT_PROFILER_H_

