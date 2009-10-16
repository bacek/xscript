#include "settings.h"

#include "xscript/control_extension.h"
#include "xscript/xml.h"
#include "xscript/xml_util.h"

#include "internal/response_time_counter_block.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

ResponseTimeCounterBlock::ResponseTimeCounterBlock(
        const ControlExtension *ext, Xml *owner, xmlNodePtr node,
        const boost::shared_ptr<ResponseTimeCounter> &counter) :
    Block(ext, owner, node), responseCounter_(counter) {
}

XmlDocHelper
ResponseTimeCounterBlock::call(boost::shared_ptr<Context> ctx, boost::any &) throw (std::exception) {
    ControlExtension::setControlFlag(ctx.get());
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    XmlNodeHelper root = responseCounter_->createReport();
    xmlDocSetRootElement(doc.get(), root.release());
    return doc;
}

ResetResponseTimeCounterBlock::ResetResponseTimeCounterBlock(
        const ControlExtension *ext, Xml *owner, xmlNodePtr node,
        const boost::shared_ptr<ResponseTimeCounter> &counter) :
    Block(ext, owner, node), responseCounter_(counter) {
}

XmlDocHelper
ResetResponseTimeCounterBlock::call(boost::shared_ptr<Context> ctx, boost::any &) throw (std::exception) {
    ControlExtension::setControlFlag(ctx.get());
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());
    XmlNodeHelper root(xmlNewNode(0, (const xmlChar*)"done"));
    XmlUtils::throwUnless(NULL != root.get());
    responseCounter_->reset();
    xmlDocSetRootElement(doc.get(), root.release());
    return doc;
}

}
