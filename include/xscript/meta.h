#ifndef _XSCRIPT_META_H_
#define _XSCRIPT_META_H_

#include <map>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

#include "xscript/xml_helpers.h"

namespace xscript {

class Meta {
public:
    Meta();
    ~Meta();

    const std::string& get(const std::string &name, const std::string &default_value) const;
    bool has(const std::string &name) const;
    void set(const std::string &name, const std::string &value);
    void set2Core(const std::string &name, const std::string &value);

    int getElapsedTime() const;
    void setElapsedTime(int time);
    static int defaultElapsedTime();

    class Core;
    boost::shared_ptr<Core> getCore() const;
    void setCore(const boost::shared_ptr<Core> &base);

    XmlNodeHelper getXml() const;
private:
    bool allowKey(const std::string &key) const;

private:
    typedef std::map<std::string, std::string> MapType;

public:
    class Core {
    public:
        Core();
        ~Core();
        bool parse(const char *buf, boost::uint32_t size);
        void serialize(std::string &buf);
        friend class Meta;
    private:
        void reset();
    private:
        MapType data_;
        int elapsed_time_;
    };

private:
    boost::shared_ptr<Core> core_;
    MapType child_;
};

} // namespace xscript

#endif // _XSCRIPT_META_H_
