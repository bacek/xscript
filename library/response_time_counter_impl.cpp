#include "settings.h"

#include <boost/lexical_cast.hpp>

#include "xscript/authorizer.h"
#include "xscript/context.h"
#include "internal/response_time_counter_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {
    
ResponseTimeCounterImpl::StatusInfo::StatusInfo()
{}

void
ResponseTimeCounterImpl::StatusInfo::add(const char *info, boost::uint64_t value) {
    boost::shared_ptr<AverageCounter> counter;
    std::string auth(info ? info : AuthContext::NOAUTH_STATUS.c_str());
    CounterMapType::iterator it = custom_counters_.find(auth);
    if (custom_counters_.end() == it) {
        counter = boost::shared_ptr<AverageCounter>(
            AverageCounterFactory::instance()->createCounter("point").release());
        custom_counters_.insert(std::make_pair(auth, counter));
    }
    else {
        counter = it->second;
    }
    counter->add(value);
}

void
ResponseTimeCounterImpl::StatusInfo::createReport(xmlNodePtr root) {
    for(CounterMapType::iterator it = custom_counters_.begin();
        it != custom_counters_.end();
        ++it) {
        XmlNodeHelper custom = it->second->createReport();
        xmlSetProp(custom.get(), (const xmlChar*)"auth-type", (const xmlChar*) it->first.c_str());
        xmlAddChild(root, custom.release());
    }
}

ResponseTimeCounterImpl::ResponseTimeCounterImpl(const std::string &name)
        : CounterImpl(name), reset_time_(time(NULL)) {
}

XmlNodeHelper
ResponseTimeCounterImpl::createReport() const {
    XmlNodeHelper root(xmlNewNode(0, (const xmlChar*) name_.c_str()));
    boost::mutex::scoped_lock lock(mtx_);
    xmlSetProp(root.get(), (const xmlChar*)"collect-time",
            (const xmlChar*) boost::lexical_cast<std::string>(time(NULL) - reset_time_).c_str());
    for(StatusCounterMapType::const_iterator it = status_counters_.begin();
        it != status_counters_.end();
        ++it) {
        XmlNodeHelper status(xmlNewNode(0, (const xmlChar*)"status"));
        xmlSetProp(status.get(), (const xmlChar*)"code",
                (const xmlChar*) boost::lexical_cast<std::string>(it->first).c_str());
        it->second->createReport(status.get());
        xmlAddChild(root.get(), status.release());
    }
    return root;
}

void
ResponseTimeCounterImpl::add(const Response *resp, boost::uint64_t value) {
    boost::mutex::scoped_lock lock(mtx_);
    boost::shared_ptr<StatusInfo> status_info = findStatusInfo(resp->status());   
    status_info->add(NULL, value);
}

void
ResponseTimeCounterImpl::add(const Context *ctx, boost::uint64_t value) {
    boost::mutex::scoped_lock lock(mtx_);
    boost::shared_ptr<StatusInfo> status_info =
        findStatusInfo(ctx->response()->status());
    status_info->add(ctx->authContext()->status().c_str(), value);
}

boost::shared_ptr<ResponseTimeCounterImpl::StatusInfo>
ResponseTimeCounterImpl::findStatusInfo(unsigned short status) {
    boost::shared_ptr<StatusInfo> status_info;
    StatusCounterMapType::iterator it = status_counters_.find(status);
    if (status_counters_.end() == it) {
        status_info = boost::shared_ptr<StatusInfo>(new StatusInfo());
        status_counters_.insert(std::make_pair(status, status_info));
    }
    else {
        status_info = it->second;
    }
    return status_info;
}

void
ResponseTimeCounterImpl::reset() {
    boost::mutex::scoped_lock lock(mtx_);
    status_counters_.clear();
    reset_time_ = time(NULL);
}

}
