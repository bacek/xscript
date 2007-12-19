#ifndef _XSCRIPT_PARAM_H_
#define _XSCRIPT_PARAM_H_

#include <string>
#include <memory>
#include <boost/utility.hpp>
#include <libxml/tree.h>

namespace xscript
{

class Object;
class Context;
class ArgList;

class Param : private boost::noncopyable
{
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

protected:
	virtual void property(const char *name, const char *value);

private:
	xmlNodePtr node_;
	std::string id_, value_;
};

class TypedParam : public Param
{
public:
	TypedParam(Object *owner, xmlNodePtr node);
	virtual ~TypedParam();
	
	virtual const std::string& value() const;

	inline const std::string& as() const {
		return as_;
	}
	
	inline const std::string& defaultValue() const {
		return default_value_;
	}
	
	virtual void add(const Context *ctx, ArgList &al) const;
	
protected:
	virtual void property(const char *name, const char *value);
	
private:
	std::string as_, default_value_;
};

typedef std::auto_ptr<Param> (*ParamCreator)(Object *owner, xmlNodePtr node);

class CreatorRegisterer : private boost::noncopyable
{
public:
	CreatorRegisterer(const char *name, ParamCreator c);
};

} // namespace xscript

#endif // _XSCRIPT_PARAM_H_
