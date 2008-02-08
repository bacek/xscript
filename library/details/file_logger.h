#ifndef _XSCRIPT_FILE_LOGGER_
#define _XSCRIPT_FILE_LOGGER_

#include "xscript/logger.h"

namespace xscript
{

/**
 * Logger implementation to output into file
 */
class FileLogger : public Logger
{
public:
    /**
     * Create logger. 
     * \param key Config key optained via subKeys
     */
    FileLogger(Logger::LogLevel level, const Config * config, const std::string& key);
    ~FileLogger();

protected:
	virtual void critInternal(const char *format, va_list args);
	virtual void errorInternal(const char *format, va_list args);
	virtual void warnInternal(const char *format, va_list args);
	virtual void infoInternal(const char *format, va_list args);
	virtual void debugInternal(const char *format, va_list args);

private:
    // File descriptor
	int fd_;

    void prepareFormat(char * buf, const char* type, const char* format);
    void write(const char* format, va_list args) const;
};

}

#endif
