#include "settings.h"

#include <iostream>
#include <map>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <libxslt/xsltutils.h>

#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/request.h"
#include "xscript/stylesheet.h"
#include "xscript/stylesheet_factory.h"
#include "xscript/writer.h"
#include "xscript/xml_util.h"

#include "xslt_profiler.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

extern "C" int closeProfileFunc(void *ctx);
extern "C" int writeProfileFunc(void *ctx, const char *data, int len);

OfflineXsltProfiler::OfflineXsltProfiler()
{}

OfflineXsltProfiler::OfflineXsltProfiler(const std::string& xslt_path) :
    xslt_path_(xslt_path)
{}

OfflineXsltProfiler::~OfflineXsltProfiler() {
    for (std::multimap<std::string, xmlDocPtr>::const_iterator it = docs_.begin();
            it != docs_.end();
            ++it) {
        xmlFreeDoc(it->second);
    }
}

void
OfflineXsltProfiler::insertProfileDoc(const std::string& name, xmlDocPtr doc) {
    boost::mutex::scoped_lock lock(mutex_);
    docs_.insert(std::pair<std::string, xmlDocPtr>(name, doc));
}

void
OfflineXsltProfiler::dumpProfileInfo(boost::shared_ptr<Context> ctx) {
    boost::mutex::scoped_lock lock(mutex_);

    std::cout << std::endl << "XSLT PROFILE INFO" << std::endl;

    boost::shared_ptr<Stylesheet> stylesheet(static_cast<Stylesheet*>(NULL));

    if (!xslt_path_.empty()) {
        namespace fs = boost::filesystem;
        fs::path path(xslt_path_);
        if (fs::exists(path) && !fs::is_directory(path)) {
            stylesheet = StylesheetFactory::createStylesheet(xslt_path_);
        }
        else {
            std::cout << "Cannot find profile stylesheet " << xslt_path_ << ". Use default xml output." << std::endl;
            log()->info("Cannot find profile stylesheet %s. Use default xml output.", xslt_path_.c_str());
        }
    }

    for (std::multimap<std::string, xmlDocPtr>::const_iterator it = docs_.begin();
            it != docs_.end();
            ++it) {
        xmlOutputBufferPtr buf = xmlOutputBufferCreateIO(&writeProfileFunc, &closeProfileFunc, ctx.get(), NULL);
        XmlUtils::throwUnless(NULL != buf);

        std::cout << std::endl << it->first << std::endl;

        XmlDocHelper doc(xmlCopyDoc(it->second, 1));
        if (NULL != stylesheet.get()) {
            doc = stylesheet->apply(NULL, ctx, boost::shared_ptr<InvokeContext>(), doc.get());
            xsltSaveResultTo(buf, doc.get(), stylesheet->stylesheet());
            xmlOutputBufferClose(buf);
        }
        else {
            xmlSaveFormatFileTo(buf, doc.get(), NULL, 1);
        }
    }
}

extern "C" int
closeProfileFunc(void *ctx) {
    (void)ctx;
    return 0;
}

extern "C" int
writeProfileFunc(void *ctx, const char *data, int len) {
    if (0 == len) {
        return 0;
    }
    try {
        std::cout.write(data, len);
        return len;
    }
    catch (const std::exception &e) {
        Context *context = static_cast<Context*>(ctx);
        log()->error("caught exception while writing result: %s %s",
                     context->request()->getScriptFilename().c_str(), e.what());
    }
    return -1;
}

} // namespace xscript
