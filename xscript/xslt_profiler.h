#ifndef _XSCRIPT_OFFLINE_XSLT_PROFILER_H_
#define _XSCRIPT_OFFLINE_XSLT_PROFILER_H_

#include <map>
#include <boost/thread/mutex.hpp>

#include "xscript/xslt_profiler.h"

namespace xscript {

class OfflineXsltProfiler : public XsltProfiler {
public:
    OfflineXsltProfiler();
    OfflineXsltProfiler(const std::string& xslt_path);
    virtual ~OfflineXsltProfiler();

    virtual void init(const Config *config);
    virtual void insertProfileDoc(const std::string& name, xmlDocPtr doc);
    virtual void dumpProfileInfo(Context* ctx);

private:
    boost::mutex mutex_;
    std::string xslt_path_;
    std::multimap<std::string, xmlDocPtr> docs_;
};

}

#endif //_XSCRIPT_OFFLINE_XSLT_PROFILER_H_

