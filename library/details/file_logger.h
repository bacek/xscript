#ifndef _XSCRIPT_FILE_LOGGER_
#define _XSCRIPT_FILE_LOGGER_

#include <vector>
#include <string>
#include <boost/thread.hpp>
#include "xscript/logger.h"

namespace xscript {

/**
 * Logger implementation to output into file
 */
class FileLogger : public Logger {
public:
    /**
     * Create logger.
     * \param key Config key optained via subKeys
     */
    FileLogger(Logger::LogLevel level, const Config * config, const std::string& key);
    ~FileLogger();

    virtual void logRotate();

protected:
    virtual void critInternal(const char *format, va_list args);
    virtual void errorInternal(const char *format, va_list args);
    virtual void warnInternal(const char *format, va_list args);
    virtual void infoInternal(const char *format, va_list args);
    virtual void debugInternal(const char *format, va_list args);

private:
    // File name
    std::string filename_;

    // Open mode
    mode_t openMode_;

    // Crash on error
    bool crash_;

    // File descriptor
    int fd_;

    // Lock of file descriptor to avoid logrotate race-condition
    boost::mutex	fdMutex_;


    // Writing queue.
    // All writes happens in separate thread. All someInternal methods just
    // push string into queue and signal conditional variable.

    // Logger is stopping.
    volatile bool stopping_;

    // Writing queue.
    std::vector<std::string>	queue_;

    // Condition and mutex for signalling.
    boost::condition			queueCondition_;
    boost::mutex				queueMutex_;

    // Writing thread.
    boost::thread				writingThread_;


    void openFile();
    static void prepareFormat(char * buf, size_t size, const char* type, const char* format);
    void pushIntoQueue(const char* type, const char* format, va_list args);

    void writingThread();
};

}

#endif
