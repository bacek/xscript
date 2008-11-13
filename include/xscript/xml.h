#ifndef _XSCRIPT_XML_H_
#define _XSCRIPT_XML_H_

#include <ctime>
#include <map>
#include <string>
#include <boost/noncopyable.hpp>

namespace xscript {

class XmlInfoCollector;

class Xml : private boost::noncopyable {
public:
    Xml();
    virtual ~Xml();

    virtual void parse() = 0;
    virtual time_t modified() const = 0;
    virtual const std::string& name() const = 0;
    virtual std::string fullName(const std::string &name) const;

    typedef std::map<std::string, time_t> TimeMapType;
    virtual const TimeMapType& modifiedInfo() const;

protected:
    virtual void swapModifiedInfo(TimeMapType &info);
        
    class IncludeModifiedTimeSetter {
    public:
        IncludeModifiedTimeSetter(Xml *xml);
        ~IncludeModifiedTimeSetter();
    private:
        Xml *xml_;
    };

    friend class IncludeModifiedTimeSetter;

private:
    TimeMapType modified_info_;
};

} // namespace xscript

#endif // _XSCRIPT_XML_H_
