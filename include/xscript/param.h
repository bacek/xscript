#ifndef _XSCRIPT_PARAM_H_
#define _XSCRIPT_PARAM_H_

#include <string>
#include <memory>
#include <boost/utility.hpp>
#include <libxml/tree.h>

namespace xscript {

class Object;
class Context;
class ArgList;
class TypedValue;
class Validator;

/**
 * Base class for block's params. Examples of concrete params are constants,
 * query args, state args, etc.
 */
class Param : private boost::noncopyable {
public:
    Param(Object *owner, xmlNodePtr node);
    virtual ~Param();

    virtual void parse();

    const std::string& id() const;
    virtual const char* type() const = 0;
    virtual const std::string& value() const;
    
    bool multi() const;

    virtual bool constant() const;

    virtual std::string asString(const Context *ctx) const = 0;
    virtual void add(const Context *ctx, ArgList &al) const = 0;

    void visitProperties();

    // Check validator (if any). Will throw exception if validation fail.
    void checkValidator(const Context *ctx) const;

protected:
    virtual void property(const char *name, const char *value);

    void multi(bool enable);

private:
    class ParamData;
    std::auto_ptr<ParamData> data_;
};

class ConvertedParam : public Param {
public:
    ConvertedParam(Object *owner, xmlNodePtr node);
    virtual ~ConvertedParam();

    const std::string& as() const;

    virtual void add(const Context *ctx, ArgList &al) const;

protected:
    virtual void property(const char *name, const char *value);

private:
    class ConvertedParamData;
    std::auto_ptr<ConvertedParamData> converted_data_;
};

class TypedParam : public ConvertedParam {
public:
    TypedParam(Object *owner, xmlNodePtr node);
    virtual ~TypedParam();

    const std::string& defaultValue() const;
    const TypedValue& defaultTypedValue() const;

    virtual void add(const Context *ctx, ArgList &al) const;
    virtual const std::string& value() const;
    virtual std::string asString(const Context *ctx) const;

    bool getStringValue(const Context *ctx, std::string &result) const;

protected:
    void addTypedValue(ArgList &al, const TypedValue &value, bool add_nil) const;

    virtual void property(const char *name, const char *value);
    typedef std::pair<std::string, bool> ValueResult;
    virtual ValueResult getValue(const Context *ctx) const;

private:
    class TypedParamData;
    std::auto_ptr<TypedParamData> typed_data_;
};

typedef std::auto_ptr<Param> (*ParamCreator)(Object *owner, xmlNodePtr node);

class CreatorRegisterer : private boost::noncopyable {
public:
    CreatorRegisterer(const char *name, ParamCreator c);
};

} // namespace xscript

#endif // _XSCRIPT_PARAM_H_
