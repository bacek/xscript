#include "xscript/stat_builder.h"
#include "xscript/counter_base.h"
#include "xscript/xml_util.h"

namespace xscript {

StatBuilder::StatBuilder(const std::string& name)
        : name_(name) {
}

StatBuilder::~StatBuilder() {
}


XmlDocHelper StatBuilder::createReport() const {
    XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
    XmlUtils::throwUnless(NULL != doc.get());

    xmlNodePtr root = xmlNewDocNode(doc.get(), NULL, (const xmlChar*) name_.c_str(), NULL);
    XmlUtils::throwUnless(NULL != root);

    xmlDocSetRootElement(doc.get(), root);

    for (std::vector<const CounterBase*>::const_iterator c = counters_.begin(); c != counters_.end(); ++c) {
        const CounterBase * cnt = *c;
        XmlNodeHelper line = cnt->createReport();
        xmlAddChild(root, line.get());
        line.release();
    }

    return doc;
}

void StatBuilder::addCounter(const CounterBase* counter) {
    assert(counter);
    counters_.push_back(counter);
}

std::auto_ptr<Block> StatBuilder::createBlock(const Extension *ext, Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new StatBuilderBlock(ext, owner, node, *this));
}

const std::string& StatBuilder::getName() const {
    return name_;
}

void StatBuilder::setName(const std::string& name) {
    name_ = name;
}

}
