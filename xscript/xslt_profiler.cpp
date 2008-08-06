#include "settings.h"

#include <iostream>
#include <map>

#include <libxslt/xsltutils.h>

#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/logger.h"
#include "xscript/request.h"
#include "xscript/stylesheet.h"
#include "xscript/util.h"
#include "xscript/writer.h"

#include "xslt_profiler.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

extern "C" int closeProfileFunc(void *ctx);
extern "C" int writeProfileFunc(void *ctx, const char *data, int len);

OfflineXsltProfiler::OfflineXsltProfiler() {
}

OfflineXsltProfiler::OfflineXsltProfiler(const std::string& xslt_path) :
	xslt_path_(xslt_path)
{}

OfflineXsltProfiler::~OfflineXsltProfiler() {
	for(std::multimap<std::string, xmlDocPtr>::const_iterator it = docs_.begin();
		it != docs_.end();
		++it) {
		xmlFreeDoc(it->second);
	}
}

void
OfflineXsltProfiler::init(const Config *config) {
	xslt_path_ = config->as<std::string>("/xscript/offline/xslt-profile-path",
		"/etc/share/xscript/profile.xsl");
}

void
OfflineXsltProfiler::insertProfileDoc(const std::string& name, xmlDocPtr doc) {
	boost::mutex::scoped_lock lock(mutex_);
	docs_.insert(std::pair<std::string, xmlDocPtr>(name, doc));
}

void
OfflineXsltProfiler::dumpProfileInfo(Context* ctx) {
	boost::mutex::scoped_lock lock(mutex_);

	xmlOutputBufferPtr buf = xmlOutputBufferCreateIO(&writeProfileFunc, &closeProfileFunc, ctx, NULL);
	XmlUtils::throwUnless(NULL != buf);

	std::cout << std::endl << "XSLT PROFILE INFO" << std::endl;

	boost::shared_ptr<Stylesheet> stylesheet(static_cast<Stylesheet*>(NULL));
	stylesheet = Stylesheet::create(xslt_path_);

	for(std::multimap<std::string, xmlDocPtr>::const_iterator it = docs_.begin();
		it != docs_.end();
		++it) {
		XmlDocHelper doc(xmlCopyDoc(it->second, 1));
		if (NULL != stylesheet.get()) {
			doc = stylesheet->apply(NULL, ctx, doc);
		}
		std::cout << std::endl << it->first << std::endl;
		xsltSaveResultTo(buf, doc.get(), stylesheet->stylesheet());
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
