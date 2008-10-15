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

    inline const std::string& id() const {
        return id_;
    }

    virtual const std::string& value() const;

    virtual std::string asString(const Context *ctx) const = 0;
    virtual void add(const Context *ctx, ArgList &al) const = 0;

    // Check validator (if any). Will throw exception if validation fail.
    void checkValidator(const Context *ctx) const;

protected:
    virtual void property(const char *name, const char *value);

private:
    xmlNodePtr node_;
    std::string id_, value_;
    std::auto_ptr<Validator> validator_;
};

class ConvertedParam : public Param {
public:
    ConvertedParam(Object *owner, xmlNodePtr node);
    virtual ~ConvertedParam();

    inline const std::string& as() const {
        return as_;
    }

    virtual void add(const Context *ctx, ArgList &al) const;

protected:
    virtual void property(const char *name, const char *value);

private:
    std::string as_;
};

class TypedParam : public ConvertedParam {
public:
    TypedParam(Object *owner, xmlNodePtr node);
    virtual ~TypedParam();

    inline const std::string& defaultValue() const {
        return default_value_;
    }

    virtual const std::string& value() const;

protected:
    virtual void property(const char *name, const char *value);

private:
    std::string default_value_;
};

typedef std::auto_ptr<Param> (*ParamCreator)(Object *owner, xmlNodePtr node);

class CreatorRegisterer : private boost::noncopyable {
public:
    CreatorRegisterer(const char *name, ParamCreator c);
};

} // namespace xscript

#endif // _XSCRIPT_PARAM_H_
