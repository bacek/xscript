#ifndef _XSCRIPT_OFFLINE_XSLT_PROFILER_H_
#define _XSCRIPT_OFFLINE_XSLT_PROFILER_H_

#include <map>
#include <boost/thread/mutex.hpp>

#include "xscript/xslt_profiler.h"
#include "xscript/xml_helpers.h"

namespace xscript {

class OfflineXsltProfiler : public XsltProfiler {
public:
    OfflineXsltProfiler();
    OfflineXsltProfiler(const std::string& xslt_path);
    virtual ~OfflineXsltProfiler();

    virtual void insertProfileDoc(const std::string& name, xmlDocPtr doc);
    virtual void dumpProfileInfo(boost::shared_ptr<Context> ctx);

private:
    boost::mutex mutex_;
    std::string xslt_path_;
    std::multimap<std::string, XmlDocSharedHelper> docs_;
};

}

#endif //_XSCRIPT_OFFLINE_XSLT_PROFILER_H_

