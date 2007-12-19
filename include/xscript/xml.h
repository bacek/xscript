#ifndef _XSCRIPT_XML_H_
#define _XSCRIPT_XML_H_

#include <ctime>
#include <string>
#include <boost/utility.hpp>

namespace xscript
{

class Config;

class Xml : private boost::noncopyable
{
public:
	Xml();
	virtual ~Xml();
	
	virtual void parse() = 0;
	virtual time_t modified() const = 0;
	virtual const std::string& name() const = 0;
	virtual std::string fullName(const std::string &name) const;
};

} // namespace xscript

#endif // _XSCRIPT_XML_H_
