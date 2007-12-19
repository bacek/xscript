#ifndef _XSCRIPT_COOKIE_H_
#define _XSCRIPT_COOKIE_H_

#include <ctime>
#include <string>

namespace xscript
{

class Cookie
{
public:
	Cookie();
	Cookie(const std::string &name, const std::string &value);
	virtual ~Cookie();
	
	inline const std::string& name() const {
		return name_;
	}
	
	inline const std::string& value() const {
		return value_;
	}
	
	inline bool secure() const {
		return secure_;
	}
	
	inline void secure(bool value) {
		secure_ = value;
	}
	
	inline time_t expires() const {
		return expires_;
	}
	
	inline void expires(time_t value) {
		expires_ = value;
	}

	inline const std::string& path() const {
		return path_;
	}
	
	inline void path(const std::string &value) {
		path_ = value;
	}
	
	inline const std::string& domain() const {
		return domain_;
	}
	
	inline void domain(const std::string &value) {
		domain_ = value;
	}

	std::string toString() const;

private:
	bool secure_;
	time_t expires_;
	std::string name_, value_, path_, domain_;
};

} // namespace xscript

#endif // _XSCRIPT_COOKIE_H_
