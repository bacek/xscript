#ifndef _XSCRIPT_ARG_LIST_H_
#define _XSCRIPT_ARG_LIST_H_

#include <string>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

namespace xscript {

class Context;
class TaggedBlock;

class ArgList : private boost::noncopyable {
public:
    ArgList();
    virtual ~ArgList();

    virtual void add(bool value) = 0;
    virtual void add(double value) = 0;

    virtual void add(boost::int32_t value) = 0;
    virtual void add(boost::int64_t value) = 0;

    virtual void add(boost::uint32_t value) = 0;
    virtual void add(boost::uint64_t value) = 0;

    virtual void add(const std::string &value) = 0;
    virtual void addAs(const std::string &type, const std::string &value) = 0;

    virtual void addState(const Context *ctx) = 0;
    virtual void addRequest(const Context *ctx) = 0;
    virtual void addRequestData(const Context *ctx) = 0;
    virtual void addTag(const TaggedBlock *tb, const Context *context) = 0;

};

} // namespace xscript

#endif // _XSCRIPT_ARG_LIST_H_
