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
    
    virtual void subKeys(const std::string &value, std::vector<std::string> &v) const = 0;

    static std::auto_ptr<Config> create(const char *file);
    static std::auto_ptr<Config> create(int &argc, char *argv[], bool dont_check = false, HelpFunc func = NULL);
    
    //TODO: make these methods non-static
    static bool getCacheParam(const std::string &name, std::string &value);
    static void addCacheParam(const std::string &name, const std::string &value);
    static void addForbiddenKey(const std::string &key);
    static void stopCollectCache();
    static const std::string& fileName();
protected:
    virtual std::string value(const std::string &value) const = 0;
};

template<typename T> inline T
Config::as(const std::string &name) const {
    std::string val = value(name);
    addCacheParam(name, val);
    return boost::lexical_cast<T>(val);
}

template<> inline std::string
Config::as<std::string>(const std::string &name) const {
    std::string val = value(name);
    addCacheParam(name, val);
    return val;
}

template<typename T> inline T
Config::as(const std::string &name, const T &defval) const {
    try {
        return as<T>(name);
    }
    catch (const std::exception &e) {
        addCacheParam(name, boost::lexical_cast<std::string>(defval));
        return defval;
    }
}

class ConfigParams {
public:
    static void init(const Config *config);
    static int defaultTimeout();
};

} // namespace xscript

#endif // _XSCRIPT_CONFIG_H_
