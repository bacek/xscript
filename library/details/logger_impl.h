#ifndef _XSCRIPT_DETAILS_LOGGER_IMPL_H_
#define _XSCRIPT_DETAILS_LOGGER_IMPL_H_

#include "xscript/logger.h"

namespace xscript
{

class LoggerImpl : public Logger
{
public:
	LoggerImpl();
	virtual ~LoggerImpl();
	
	virtual unsigned int level() const;
	virtual void level(unsigned int value);
	
	virtual void init(const Config *config);
	
protected:
	virtual void warnInternal(const char *format, va_list args);
	virtual void infoInternal(const char *format, va_list args);
	virtual void errorInternal(const char *format, va_list args);
	virtual void debugInternal(const char *format, va_list args);

private:
	std::string ident_;
	volatile unsigned int level_;
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_LOGGER_IMPL_H_
