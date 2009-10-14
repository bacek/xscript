#ifndef _XSCRIPT_INTERNAL_RESPONSE_TIME_COUNTER_IMPL_H
#define _XSCRIPT_INTERNAL_RESPONSE_TIME_COUNTER_IMPL_H

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "xscript/average_counter.h"
#include "xscript/response_time_counter.h"
#include "xscript/xml_util.h"

#include "internal/counter_impl.h"

namespace xscript {

/**
 * Counter for measure response time statistic.
 */
class ResponseTimeCounterImpl : virtual public ResponseTimeCounter, virtual public CounterImpl {
public:
    ResponseTimeCounterImpl(const std::string &name);
    
    virtual void add(const Response *resp, boost::uint64_t value);
    virtual void add(const Context *ctx, boost::uint64_t value);
    virtual XmlNodeHelper createReport() const;
    virtual void reset();
    
    struct StatusInfo {       
        StatusInfo();
        void add(const char *info, boost::uint64_t value);
        void createReport(xmlNodePtr root);
        
        typedef std::map<std::string, boost::shared_ptr<AverageCounter> > CounterMapType;
        CounterMapType custom_counters_;
    };
    
private:
    boost::shared_ptr<StatusInfo> findStatusInfo(unsigned short status);
    
private:   
    typedef std::map<unsigned short, boost::shared_ptr<StatusInfo> > StatusCounterMapType;
    StatusCounterMapType status_counters_;
    time_t reset_time_;
};

}

#endif
