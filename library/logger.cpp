#include "settings.h"

#include <sstream>
#include <cstring>
#include <stdexcept>
#include <boost/static_assert.hpp>

#include <syslog.h>

#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/logger_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Logger::Logger(LogLevel level)
    : level_(level)	
{
}

Logger::~Logger() {
}

void
Logger::exiting(const char *function) {
	debug("exiting %s", function);
}

void
Logger::entering(const char *function) {
	debug("entering %s", function);
}

void
Logger::crit(const char *format, ...) {
	if (level() >= LEVEL_CRIT) {
 		va_list args;
		va_start(args, format);
		critInternal(format, args);
		va_end(args);
	}
}

void
Logger::error(const char *format, ...) {
	if (level() >= LEVEL_ERROR) {
 		va_list args;
		va_start(args, format);
		errorInternal(format, args);
		va_end(args);
	}
}

void
Logger::warn(const char *format, ...) {
	if (level() >= LEVEL_WARN) {
		va_list args;
		va_start(args, format);
		warnInternal(format, args);
		va_end(args);
	}
}

void
Logger::info(const char *format, ...) {
	if (level() >= LEVEL_INFO) {
		va_list args;
		va_start(args, format);
		infoInternal(format, args);
		va_end(args);
	}
}

void
Logger::debug(const char *format, ...) {
	if (level() >= LEVEL_DEBUG) {
		va_list args;
		va_start(args, format);
		debugInternal(format, args);
		va_end(args);
	}
}

void
Logger::xmllog(const char *format, va_list args) {
	log()->infoInternal(format, args);
}


void
Logger::xmllog(void *ctx, const char *format, ...) {
    (void)ctx;
	va_list args;
	va_start(args, format);
	xmllog(format, args);
	va_end(args);
}

Logger*
log() {
	return LoggerFactory::instance()->getDefaultLogger();
}



} // namespace xscript
