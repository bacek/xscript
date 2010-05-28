#ifndef _XSCRIPT_META_H_
#define _XSCRIPT_META_H_

#include <map>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "xscript/xml_helpers.h"

namespace xscript {

class MetaCore : private boost::noncopyable {
public:
    MetaCore();
    ~MetaCore();
    bool parse(const char *buf, boost::uint32_t size);
    void serialize(std::string &buf) const;
    static int undefinedElapsedTime();
    friend class Meta;
private:
    void reset();
private:
    typedef std::map<std::string, std::string> MapType;
    MapType data_;
    int elapsed_time_;
};

class Meta : private boost::noncopyable {
public:
    Meta();
    ~Meta();

    const std::string& get(const std::string &name, const std::string &default_value) const;
    bool has(const std::string &name) const;
    void set(const std::string &name, const std::string &value);
    void set2Core(const std::string &name, const std::string &value);

    int getElapsedTime() const;
    void setElapsedTime(int time);

    time_t getExpireTime() const;
    void setExpireTime(time_t time);

    time_t getLastModified() const;
    void setLastModified(time_t time);

    static time_t undefinedExpireTime();
    static time_t undefinedLastModified();

    boost::shared_ptr<MetaCore> getCore() const;
    void setCore(const boost::shared_ptr<MetaCore> &base);

    XmlNodeHelper getXml() const;
    void cacheParamsWritable(bool flag);

    void reset();
private:
    void initCore();
    bool allowKey(const std::string &key) const;

private:
    typedef MetaCore::MapType MapType;
    boost::shared_ptr<MetaCore> core_;
    MapType child_;
    time_t expire_time_;
    time_t last_modified_;
    bool cache_params_writable_;
};

} // namespace xscript

#endif // _XSCRIPT_META_H_
