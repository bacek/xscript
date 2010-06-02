#ifndef _XSCRIPT_XML_H_
#define _XSCRIPT_XML_H_

#include <ctime>
#include <map>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>

namespace xscript {

class Xml : private boost::noncopyable {
public:
    Xml(const std::string &name);
    virtual ~Xml();

    const std::string& name() const;
    std::string fullName(const std::string &name) const;

    typedef std::map<std::string, time_t> TimeMapType;
    const TimeMapType& modifiedInfo() const;
    void swapModifiedInfo(TimeMapType &info);

private:
    class XmlData;
    std::auto_ptr<XmlData> data_;
};

} // namespace xscript

#endif // _XSCRIPT_XML_H_
