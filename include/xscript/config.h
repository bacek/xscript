#ifndef _XSCRIPT_CONFIG_H_
#define _XSCRIPT_CONFIG_H_

#include <string>
#include <vector>
#include <memory>
#include <iosfwd>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>

namespace xscript {

typedef std::ostream& (*HelpFunc)(std::ostream &stream);

class Config : private boost::noncopyable {
public:
    Config();
    virtual ~Config();

    virtual void startup();

    template<typename T> T as(const std::string &name) const;
    template<typename T> T as(const std::string &name, const T &defval) const;

    virtual std::string value(const std::string &value) const = 0;
    virtual void subKeys(const std::string &value, std::vector<std::string> &v) const = 0;

    static std::auto_ptr<Config> create(const char *file);
    static std::auto_ptr<Config> create(int &argc, char *argv[], HelpFunc func = NULL);
};

template<typename T> inline T
Config::as(const std::string &name) const {
    return boost::lexical_cast<T>(value(name));
}

template<> inline std::string
Config::as<std::string>(const std::string &name) const {
    return value(name);
}

template<typename T> inline T
Config::as(const std::string &name, const T &defval) const {
    try {
        return as<T>(name);
    }
    catch (const std::exception &e) {
        return defval;
    }
}

} // namespace xscript

#endif // _XSCRIPT_CONFIG_H_
