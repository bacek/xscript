#ifndef _XSCRIPT_STAT_BUILDER_H_
#define _XSCRIPT_STAT_BUILDER_H_

#include <vector>
#include <string>
#include <libxml/tree.h>

#include <boost/bind.hpp>

#include <xscript/xml_helpers.h>
#include <xscript/block.h>
#include <xscript/control_extension.h>


namespace xscript {

class CounterBase;

/**
 * Statistic builder. Holds pointers to CountersBase and build
 * aggregate xmlNodePtr by calling CounterBase::createReport for each
 * counter and wrap them into single node.
 *
 * NB: counters are not owned by StatBuilder
 */
class StatBuilder {
public:
    StatBuilder(const std::string& name);
    virtual ~StatBuilder();

    /**
     * Create aggregate report. Caller must free result.
     */
    XmlDocHelper createReport() const;

    /**
     * Add counter to report.
     */
    void addCounter(const CounterBase* counter);

    /**
     * Create block to output statistic in xscript page.
     */
    std::auto_ptr<Block> createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node);

    const std::string& getName() const;
    void setName(const std::string& name);

private:
    std::string name_;
    std::vector<const CounterBase*>	counters_;
};

class StatBuilderBlock : public Block {
public:
    StatBuilderBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node, const StatBuilder& builder)
            : Block(ext, owner, node), builder_(builder) {
    }

    XmlDocHelper call(Context *, boost::any &) throw (std::exception) {
        return builder_.createReport();
    }
private:
    const StatBuilder &builder_;
};

class StatBuilderHolder {
public:
    StatBuilderHolder(const std::string& name) : statBuilder_(name) {
        ControlExtension::Constructor f =
            boost::bind(boost::mem_fn(&StatBuilder::createBlock), &statBuilder_, _1, _2, _3);
        ControlExtension::registerConstructor(statBuilder_.getName() + "-stat", f);
    }

    virtual ~StatBuilderHolder() {};

protected:
    StatBuilder& getStatBuilder() {
        return statBuilder_;
    }

private:
    StatBuilder statBuilder_;
};

} // namespace xscript

#endif // _XSCRIPT_STAT_BUILDER_H_
