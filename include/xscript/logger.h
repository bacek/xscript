#ifndef _XSCRIPT_LOGGER_H_
#define _XSCRIPT_LOGGER_H_

#include <cstdarg>
#include <xscript/component.h>

namespace xscript
{

class Logger : 
	public virtual Component,
	public ComponentHolder<Logger>
{
public:
	Logger();
	virtual ~Logger();
	
	void exiting(const char *function);
	void entering(const char *function);

	void warn(const char *format, ...);
	void info(const char *format, ...);

	void error(const char *format, ...);
	void debug(const char *format, ...);
	
	static Logger* createImpl();
	static void xmllog(const char *format, va_list args);
	static void xmllog(void *ctx, const char *format, ...);

	virtual unsigned int level() const = 0;
	virtual void level(unsigned int value) = 0;
	virtual void init(const Config *config) = 0;
	
	static const unsigned int LEVEL_ERROR = 1;
	static const unsigned int LEVEL_WARN  = 2;
	static const unsigned int LEVEL_INFO  = 3;
	static const unsigned int LEVEL_DEBUG = 4;

protected:
	virtual void warnInternal(const char *format, va_list args) = 0;
	virtual void infoInternal(const char *format, va_list args) = 0;
	
	virtual void errorInternal(const char *format, va_list args) = 0;
	virtual void debugInternal(const char *format, va_list args) = 0;
};

Logger* log();

} // namespace xscript

#endif // _XSCRIPT_LOGGER_H_
