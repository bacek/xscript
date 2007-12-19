#ifndef _XSCRIPT_STATE_VALUE_H_
#define _XSCRIPT_STATE_VALUE_H_

#include <string>

namespace xscript
{

class StateValue
{
public:
	StateValue();
	StateValue(int type, const std::string &value);
	
	inline int type() const {
		return type_;
	}

	inline const std::string& value() const {
		return value_;
	}
	
	static const int TYPE_BOOL = 1;
	static const int TYPE_LONG = 1 << 1;
	static const int TYPE_ULONG = 1 << 2;
	static const int TYPE_LONGLONG = 1 << 3;
	static const int TYPE_ULONGLONG = 1 << 4;
	static const int TYPE_DOUBLE = 1 << 5;
	static const int TYPE_STRING = 1 << 6;
	
private:	
	int type_;
	std::string value_;
};

} // namespace xscript

#endif // _XSCRIPT_STATE_VALUE_H_
