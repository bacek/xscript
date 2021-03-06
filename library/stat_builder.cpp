#include "settings.h"

#include "xscript/stat_builder.h"
#include "xscript/counter_base.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class StatBuilderBlock : public Block {
public:
    StatBuilderBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node, const StatBuilder& builder)
            : Block(ext, owner, node), builder_(builder) {
    }

    void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
        ControlExtension::setControlFlag(ctx.get());
        invoke_ctx->resultDoc(builder_.createReport());
    }
private:
    const StatBuilder &builder_;
};

class StatBuilder::StatBuilderData {
public:
    StatBuilderData(const std::string& name) : name_(name) {}
    ~StatBuilderData() {}
    
    void addCounter(const CounterBase* counter) {
        counters_.push_back(counter);
    }
    
    std::string name_;
    std::vector<const CounterBase*> counters_;
};

StatBuilder::StatBuilder(const std::string& name) :
    data_(new StatBuilderData(name))
{}

StatBuilder::~StatBuilder() {
}

XmlDocHelper
StatBuilder::createReport() const {
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    xmlNodePtr root = xmlNewDocNode(doc.get(), NULL, (const xmlChar*)data_->name_.c_str(), NULL);
    XmlUtils::throwUnless(NULL != root);

    xmlDocSetRootElement(doc.get(), root);

    for(std::vector<const CounterBase*>::const_iterator c = data_->counters_.begin();
        c != data_->counters_.end();
        ++c) {
        const CounterBase * cnt = *c;
        XmlNodeHelper line = cnt->createReport();
        xmlAddChild(root, line.get());
        line.release();
    }

    return doc;
}

void
StatBuilder::addCounter(const CounterBase* counter) {
    assert(counter);
    data_->addCounter(counter);
}

std::auto_ptr<Block>
StatBuilder::createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new StatBuilderBlock(ext, owner, node, *this));
}

const std::string&
StatBuilder::getName() const {
    return data_->name_;
}

} // namespace xscript
