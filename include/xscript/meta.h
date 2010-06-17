#ifndef _XSCRIPT_META_H_
#define _XSCRIPT_META_H_

#include <map>
#include <string>

#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "xscript/typed_map.h"
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
    TypedMap data_;
    int elapsed_time_;
};

class Meta : private boost::noncopyable {
public:
    Meta();
    ~Meta();

    std::string get(const std::string &name, const std::string &default_value) const;
    bool has(const std::string &name) const;
    bool getTypedValue(const std::string &name, TypedValue &value) const;
    void setTypedValue(const std::string &name, const TypedValue &value);

    void setBool(const std::string &name, bool value);
    void setLong(const std::string &name, boost::int32_t value);
    void setULong(const std::string &name, boost::uint32_t value);
    void setLongLong(const std::string &name, boost::int64_t value);
    void setULongLong(const std::string &name, boost::uint64_t value);
    void setDouble(const std::string &name, double value);
    void setString(const std::string &name, const std::string &value);
    void setArray(const std::string &name, const std::vector<std::string> &value);
    void setMap(const std::string &name, const std::map<std::string, std::string> &value);

    template<typename Type> void set(const std::string &name, Type val);

    int getElapsedTime() const;
    void setElapsedTime(int time);

    time_t getExpireTime() const;
    void setExpireTime(time_t time);

    time_t getLastModified() const;
    void setLastModified(time_t time);

    void setCacheParams(time_t expire, time_t last_modified);

    static time_t undefinedExpireTime();
    static time_t undefinedLastModified();

    boost::shared_ptr<MetaCore> getCore() const;
    void setCore(const boost::shared_ptr<MetaCore> &base);

    XmlNodeHelper getXml() const;
    void cacheParamsWritable(bool flag);
    void coreWritable(bool flag);

    void reset();
private:
    void initCore();
    bool allowKey(const std::string &key) const;

private:
    boost::shared_ptr<MetaCore> core_;
    TypedMap child_;
    time_t expire_time_;
    time_t last_modified_;
    bool cache_params_writable_;
    bool core_writable_;
};

template<typename Type> inline void
Meta::set(const std::string &name, Type val) {
    setString(name, boost::lexical_cast<std::string>(val));
}

template<> inline void
Meta::set<bool>(const std::string &name, bool val) {
    setBool(name, val);
}

template<> inline void
Meta::set<boost::int32_t>(const std::string &name, boost::int32_t val) {
    setLong(name, val);
}

template<> inline void
Meta::set<boost::int64_t>(const std::string &name, boost::int64_t val) {
    setLongLong(name, val);
}

template<> inline void
Meta::set<boost::uint32_t>(const std::string &name, boost::uint32_t val) {
    setULong(name, val);
}

template<> inline void
Meta::set<boost::uint64_t>(const std::string &name, boost::uint64_t val) {
    setULongLong(name, val);
}

template<> inline void
Meta::set<double>(const std::string &name, double val) {
    setDouble(name, val);
}

template<> inline void
Meta::set<const std::string&>(const std::string &name, const std::string &val) {
    setString(name, val);
}

template<> inline void
Meta::set<const std::vector<std::string>&>(
    const std::string &name, const std::vector<std::string> &val) {
    setTypedValue(name, TypedValue(val));
}

} // namespace xscript

#endif // _XSCRIPT_META_H_
