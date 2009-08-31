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

    XmlDocHelper call(boost::shared_ptr<Context>, boost::any &) throw (std::exception) {
        return builder_.createReport();
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

StatBuilderHolder::StatBuilderHolder(const std::string& name) : statBuilder_(name) {
    ControlExtension::Constructor f =
        boost::bind(boost::mem_fn(&StatBuilder::createBlock), &statBuilder_, _1, _2, _3);
    ControlExtension::registerConstructor(statBuilder_.getName() + "-stat", f);
}

StatBuilderHolder::~StatBuilderHolder()
{}

StatBuilder&
StatBuilderHolder::getStatBuilder() {
    return statBuilder_;
}

StatBuilder::StatBuilder(const std::string& name) :
    data_(new StatBuilderData(name))
{}

StatBuilder::~StatBuilder() {
    delete data_;
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
