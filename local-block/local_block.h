#ifndef _XSCRIPT_LOCAL_BLOCK_H_
#define _XSCRIPT_LOCAL_BLOCK_H_

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "xscript/tagged_block.h"
#include "xscript/threaded_block.h"

namespace xscript {

class Script;

class LocalBlock : public ThreadedBlock, public TaggedBlock {
public:
    LocalBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~LocalBlock();

    virtual std::string createTagKey(const Context *ctx, const InvokeContext *invoke_ctx) const;
protected:
    virtual void parseSubNode(xmlNodePtr node);
    virtual void postParse();

    void propertyInternal(const char *name, const char *value);
    void postParseInternal();

    void proxy(bool flag);
    bool proxy() const;

    boost::shared_ptr<Script> script() const;

private:
    LocalBlock(const LocalBlock &);
    LocalBlock& operator = (const LocalBlock &);

    virtual void property(const char *name, const char *value);
    virtual void call(boost::shared_ptr<Context> ctx,
        boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception);

private:
    bool proxy_;
    boost::shared_ptr<Script> script_;
    std::string node_id_;
};

} // namespace xscript

#endif // _XSCRIPT_LOCAL_BLOCK_H_
