#include "settings.h"

#include <sstream>
#include <cstring>
#include <stdexcept>
#include <boost/static_assert.hpp>

#include <syslog.h>

#include "xscript/config.h"
#include "xscript/logger.h"
#include "details/logger_impl.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Logger::Logger()
	
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

Logger*
Logger::createImpl() {
	return new LoggerImpl();
}

void
Logger::xmllog(const char *format, va_list args) {
	instance()->infoInternal(format, args);
}

void
Logger::xmllog(void *ctx, const char *format, ...) {
	va_list args;
	va_start(args, format);
	xmllog(format, args);
	va_end(args);
}

Logger*
log() {
	return Logger::instance();
}

LoggerImpl::LoggerImpl() :
	level_(LEVEL_INFO)
{
}

LoggerImpl::~LoggerImpl() {
	closelog();
}

unsigned int
LoggerImpl::level() const {
	return level_;
}

void
LoggerImpl::level(unsigned int value) {
	if (value >= LEVEL_ERROR && value <= LEVEL_DEBUG) {
		level_ = value;
	}
	else {
		std::stringstream stream;
		stream << "requested bad log level: " << value;
		throw std::runtime_error(stream.str());
	}
}

void
LoggerImpl::init(const Config *config) {
	
	ident_ = config->as<std::string>("/xscript/logger/ident");
	std::string level = config->as<std::string>("/xscript/logger/level");
	
	if (strncasecmp(level.c_str(), "warn", sizeof("warn")) == 0) {
		this->level(LEVEL_WARN);
	}
	else if (strncasecmp(level.c_str(), "error", sizeof("error")) == 0) {
		this->level(LEVEL_ERROR);
	}
	else if (strncasecmp(level.c_str(), "debug", sizeof("debug")) == 0) {
		this->level(LEVEL_DEBUG);
	}
	else {
		this->level(LEVEL_INFO);
	}
	closelog();
	openlog(ident_.c_str(), LOG_ODELAY, LOG_DAEMON);
}

void
LoggerImpl::warnInternal(const char *format, va_list args) {
	vsyslog(LOG_WARNING, format, args);
}

void
LoggerImpl::infoInternal(const char *format, va_list args) {
	vsyslog(LOG_INFO, format, args);
}

void
LoggerImpl::errorInternal(const char *format, va_list args) {
	vsyslog(LOG_ERR, format, args);
}

void
LoggerImpl::debugInternal(const char *format, va_list args) {
	vsyslog(LOG_DEBUG, format, args);
}

} // namespace xscript
