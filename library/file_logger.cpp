#include "settings.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#include <stdexcept>

#include <boost/bind.hpp>

#include "xscript/config.h"
#include "xscript/logger.h"
#include "xscript/util.h"
#include "details/file_logger.h"


#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

const size_t BUF_SIZE = 5120;

namespace xscript {


FileLogger::FileLogger(Logger::LogLevel level, const Config * config, const std::string &key)
        :
        Logger(level),
        openMode_(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH),
        crash_(true),
        fd_(-1),
        stopping_(false),
        writingThread_(boost::bind(&FileLogger::writingThread, this)) {
    filename_ = config->as<std::string>(key + "/file");

    std::string read = config->as<std::string>(key + "/read", "");
    if (!read.empty()) {
        if (read == "all") {
            openMode_ = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        }
        else if (read == "group") {
            openMode_ = S_IRUSR | S_IWUSR | S_IRGRP;
        }
        else if (read == "user") {
            openMode_ = S_IRUSR | S_IWUSR;
        }
    }

    std::string crash = config->as<std::string>(key + "/crash-on-errors", "");
    if (crash == "no") {
        crash_ = false;
    }

    std::string::size_type pos = 0;
    while (true) {
        pos = filename_.find('/', pos + 1);
        if (std::string::npos == pos) {
            break;
        }
        FileUtils::makeDir(filename_.substr(0, pos), openMode_ | S_IXUSR | S_IXGRP | S_IXOTH);
    }
    
    openFile();
}

FileLogger::~FileLogger() {
    stopping_ = true;
    queueCondition_.notify_one();
    writingThread_.join();

    if (fd_ != -1) {
        close(fd_);
    }
}

void FileLogger::openFile() {
    boost::mutex::scoped_lock fdLock(fdMutex_);

    if (fd_ != -1) {
        close(fd_);
    }
    fd_ = open(filename_.c_str(), O_WRONLY | O_CREAT | O_APPEND, openMode_);
    if (fd_ == -1) {
        if (crash_) {
            std::string message = "Cannot open file for writing: " + filename_;
            terminate(42, message.c_str(), false);
        }
    }
}

void FileLogger::logRotate() {
    openFile();
}

void
FileLogger::critInternal(const char *format, va_list args) {
    pushIntoQueue("crit", format, args);
}

void
FileLogger::errorInternal(const char *format, va_list args) {
    pushIntoQueue("error", format, args);
}


void
FileLogger::warnInternal(const char *format, va_list args) {
    pushIntoQueue("warn", format, args);
}


void
FileLogger::infoInternal(const char *format, va_list args) {
    pushIntoQueue("info", format, args);
}

void
FileLogger::debugInternal(const char *format, va_list args) {
    pushIntoQueue("debug", format, args);
}

void
FileLogger::pushIntoQueue(const char* type, const char* format, va_list args) {

    // Check without lock!
    if (fd_ == -1)
        return;

    char fmt[BUF_SIZE];
    prepareFormat(fmt, sizeof(fmt), type, format);

    char buf[BUF_SIZE];
    int size = vsnprintf(buf, BUF_SIZE - 1, fmt, args);
    if (size > 0) {
        buf[BUF_SIZE - 1] = '\0';
        boost::mutex::scoped_lock lock(queueMutex_);
        queue_.push_back(buf);
        queueCondition_.notify_one();
    }
}

void
FileLogger::prepareFormat(char * buf, size_t size, const char* type, const char* format) {
    time_t t;
    struct tm tm;
    time(&t);
    localtime_r(&t, &tm);

    char timestr[64];
    strftime(timestr, sizeof(timestr) - 1, "[%Y/%m/%d %T]", &tm);

    snprintf(buf, size - 1, "%s %s: %s\n", timestr, type, format);
    buf[size - 1] = '\0';
}

void
FileLogger::writingThread() {
    while (!stopping_) {
        std::vector<std::string> queueCopy;

        {
            boost::mutex::scoped_lock lock(queueMutex_);
            queueCondition_.wait(lock);
            std::swap(queueCopy, queue_);
            // unlock mutex
        }

        boost::mutex::scoped_lock fdlock(fdMutex_);
        if (fd_ != -1) {
            for (std::vector<std::string>::iterator i = queueCopy.begin(); i != queueCopy.end(); ++i) {
                ::write(fd_, i->c_str(), i->length());
            }
        }
    }
}


} // namespace xscript
