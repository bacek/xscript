#ifndef _XSCRIPT_INTERNAL_RESPONSE_TIME_COUNTER_BLOCK_H
#define _XSCRIPT_INTERNAL_RESPONSE_TIME_COUNTER_BLOCK_H

#include <boost/shared_ptr.hpp>

#include "xscript/block.h"
#include "xscript/response_time_counter.h"
#include "xscript/xml_helpers.h"

namespace xscript {

class ControlExtension;
class Xml;

class ResponseTimeCounterBlock : public Block {
public:
    ResponseTimeCounterBlock(const ControlExtension *ext,
                             Xml *owner,
                             xmlNodePtr node,
                             const boost::shared_ptr<ResponseTimeCounter> &counter);
    XmlDocHelper call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception);
    
private:
    boost::shared_ptr<ResponseTimeCounter> responseCounter_;
};

class ResetResponseTimeCounterBlock : public Block {
public:
    ResetResponseTimeCounterBlock(const ControlExtension *ext,
                                  Xml *owner,
                                  xmlNodePtr node,
                                  const boost::shared_ptr<ResponseTimeCounter> &counter);
    XmlDocHelper call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception);
    
private:
    boost::shared_ptr<ResponseTimeCounter> responseCounter_;
};

}

#endif //_XSCRIPT_INTERNAL_RESPONSE_TIME_COUNTER_BLOCK_H
