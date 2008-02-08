#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <stdexcept>

#include "xscript/config.h"
#include "xscript/logger.h"
#include "details/file_logger.h"


#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

const size_t BUF_SIZE = 512;

namespace xscript
{


FileLogger::FileLogger(Logger::LogLevel level, const Config * config, const std::string &key)
    : Logger(level)
{
    std::string filename = config->as<std::string>(key + "/file");
    fd_ = open(filename.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if(fd_ == -1)
        throw std::runtime_error("Can't open file in FileLogger");
}

FileLogger::~FileLogger() {
	close(fd_);
}

void 
FileLogger::prepareFormat(char * buf, const char* type, const char* format) {
    time_t t;
    struct tm tm;
    time(&t);
    localtime_r(&t, &tm);
    
    char timestr[BUF_SIZE];
    strftime(timestr, BUF_SIZE, "[%Y/%m/%d %T]", &tm);

    snprintf(buf, BUF_SIZE, "%s %s: %s\n", timestr, type, format);
}

void 
FileLogger::write(const char* format, va_list args) const {
    char buf[BUF_SIZE];
    size_t size = vsnprintf(buf, BUF_SIZE, format, args);
    ::write(fd_, buf, size);
}

void
FileLogger::critInternal(const char *format, va_list args) {
    char fmt[BUF_SIZE];
    prepareFormat(fmt, "crit", format);
    write(fmt, args);
}

void
FileLogger::errorInternal(const char *format, va_list args) {
    char fmt[BUF_SIZE];
    prepareFormat(fmt, "error", format);
    write(fmt, args);
}


void
FileLogger::warnInternal(const char *format, va_list args) {
    char fmt[BUF_SIZE];
    prepareFormat(fmt, "warn", format);
    write(fmt, args);
}


void
FileLogger::infoInternal(const char *format, va_list args) {
    char fmt[BUF_SIZE];
    prepareFormat(fmt, "info", format);
    write(fmt, args);
}

void
FileLogger::debugInternal(const char *format, va_list args) {
    char fmt[BUF_SIZE];
    prepareFormat(fmt, "debug", format);
    write(fmt, args);
}

} // namespace xscript
