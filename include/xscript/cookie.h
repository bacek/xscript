#ifndef _XSCRIPT_COOKIE_H_
#define _XSCRIPT_COOKIE_H_

#include <ctime>
#include <memory>
#include <string>

namespace xscript {

class Cookie {
public:
    Cookie();
    Cookie(const std::string &name, const std::string &value);
    Cookie(const Cookie &cookie);
    virtual ~Cookie();
    Cookie& operator=(const Cookie &cookie);

    const std::string& name() const;
    const std::string& value() const;
    bool secure() const;
    void secure(bool value);
    bool httpOnly() const;
    void httpOnly(bool value);
    time_t expires() const;
    void expires(time_t value);
    const std::string& path() const;
    void path(const std::string &value);
    const std::string& domain() const;
    void domain(const std::string &value);
    bool permanent() const;
    void permanent(bool value);
    void urlEncode(bool value);
    std::string toString() const;
    bool check() const;
private:
    class CookieData;
    std::auto_ptr<CookieData> data_;
};

} // namespace xscript

#endif // _XSCRIPT_COOKIE_H_
