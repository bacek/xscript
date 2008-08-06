#ifndef _XSCRIPT_DETAILS_LOGGER_IMPL_H_
#define _XSCRIPT_DETAILS_LOGGER_IMPL_H_

#include "xscript/logger.h"

namespace xscript {

/**
 * Logger implementation to output using syslog facility
 */
class SyslogLogger : public Logger {
public:
    /**
     * Create logger.
     * \param key Config key optained via subKeys
     */
    SyslogLogger(Logger::LogLevel level, const Config * config, const std::string &key);
    virtual ~SyslogLogger();

    virtual void logRotate();
protected:
    virtual void critInternal(const char *format, va_list args);
    virtual void errorInternal(const char *format, va_list args);
    virtual void warnInternal(const char *format, va_list args);
    virtual void infoInternal(const char *format, va_list args);
    virtual void debugInternal(const char *format, va_list args);

private:
    std::string ident_;
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_LOGGER_IMPL_H_
