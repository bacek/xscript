#ifndef _XSCRIPT_CONFIG_H_
#define _XSCRIPT_CONFIG_H_

#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

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
    
    virtual const std::string& fileName() const = 0;

    int defaultBlockTimeout() const;

protected:
    virtual std::string value(const std::string &value) const = 0;

public:
    bool getCacheParam(const std::string &name, std::string &value) const;
    void stopCollectCache();
    void addForbiddenKey(const std::string &key) const;
    void addCacheParam(const std::string &name, const std::string &value) const;
protected:
    void initParams();

private:
    mutable std::map<std::string, std::string> cache_params_;
    mutable std::set<std::string> forbidden_keys_;
    bool stop_collect_cache_;
    int default_block_timeout_;
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
