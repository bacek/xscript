#ifndef _XSCRIPT_LOCAL_ARG_LIST_H_
#define _XSCRIPT_LOCAL_ARG_LIST_H_

#include "xscript/args.h"

#include "xscript/typed_map.h"

namespace xscript {

class LocalArgList : public ArgList {
public:
    LocalArgList();
    virtual ~LocalArgList();

    virtual void add(bool value);
    virtual void add(double value);
    virtual void add(boost::int32_t value);
    virtual void add(boost::int64_t value);
    virtual void add(boost::uint32_t value);
    virtual void add(boost::uint64_t value);
    virtual void add(const std::string &value);
    virtual void addAs(const std::string &type, const TypedValue &value);
    virtual bool empty() const;
    virtual unsigned int size() const;
    virtual const std::string& at(unsigned int i) const;

    const TypedValue& typedValue(unsigned int i) const;
private:
    void add(const TypedValue &value);

private:
    std::vector<TypedValue> args_;
};

} // namespace xscript

#endif // _XSCRIPT_LOCAL_ARG_LIST_H_
