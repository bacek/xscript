#include "settings.h"

#include <sstream>
#include <cstring>
#include <stdexcept>
#include <boost/static_assert.hpp>

#include <syslog.h>

#include "xscript/config.h"
#include "xscript/logger.h"
#include "details/syslog_logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

#include <iostream>

namespace xscript {


SyslogLogger::SyslogLogger(Logger::LogLevel level, const Config * config, const std::string &key)
        : Logger(level) {
    ident_ = config->as<std::string>(key + "/ident");
    closelog();
    openlog(ident_.c_str(), LOG_ODELAY, LOG_DAEMON);
}

SyslogLogger::~SyslogLogger() {
    closelog();
}

void
SyslogLogger::critInternal(const char *format, va_list args) {
    vsyslog(LOG_NOTICE, format, args);
}

void
SyslogLogger::errorInternal(const char *format, va_list args) {
    vsyslog(LOG_ERR, format, args);
}


void
SyslogLogger::warnInternal(const char *format, va_list args) {
    vsyslog(LOG_WARNING, format, args);
}


void
SyslogLogger::infoInternal(const char *format, va_list args) {
    vsyslog(LOG_INFO, format, args);
}

void
SyslogLogger::debugInternal(const char *format, va_list args) {
    vsyslog(LOG_DEBUG, format, args);
}

void SyslogLogger::logRotate() {
}

} // namespace xscript
