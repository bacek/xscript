#include "settings.h"

#include <iostream>
#include <boost/bind.hpp>
#include <vector>

#include "xscript/logger_factory.h"
#include "xscript/config.h"
#include "xscript/control_extension.h"
#include "xscript/request.h"
#include "xscript/vhost_data.h"
#include "xscript/xml_util.h"
#include "details/syslog_logger.h"
#include "details/file_logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

static const unsigned char FLAG_PRINT_THREAD_ID = 1;
static const unsigned char FLAG_PRINT_REQUEST_ID = 2;


LoggerFactory::LoggerFactory() : defaultLogger_(0)
{
}

LoggerFactory::~LoggerFactory() {
}

class LoggerFactoryBlock : public Block {
public:
    LoggerFactoryBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node)
            : Block(ext, owner, node) {
    }

    void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception) {
        ControlExtension::setControlFlag(ctx.get());
        LoggerFactory::instance()->logRotate();
        XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
        XmlUtils::throwUnless(NULL != doc.get());
        xmlNodePtr node = xmlNewDocNode(doc.get(), NULL, (const xmlChar*) "logrotate", (const xmlChar*) "rotated");
        XmlUtils::throwUnless(NULL != node);
        xmlDocSetRootElement(doc.get(), node);
        invoke_ctx->resultDoc(doc);
    }
};

std::auto_ptr<Block>
LoggerFactory::createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node) {
    return std::auto_ptr<Block>(new LoggerFactoryBlock(ext, owner, node));
}


void
LoggerFactory::init(const Config * config) {
    ControlExtension::Constructor f = boost::bind(boost::mem_fn(&LoggerFactory::createBlock), this, _1, _2, _3);
    ControlExtension::registerConstructor("logrotate", f);
    
    config->addForbiddenKey("/xscript/logger/*");
    
    std::vector<std::string> v;
    std::string key("/xscript/logger-factory/logger");

    config->subKeys(key, v);
    for (std::vector<std::string>::iterator i = v.begin(), end = v.end(); i != end; ++i) {
        std::string id = config->as<std::string>((*i) + "/id");
        std::string type = config->as<std::string>((*i) + "/type");
        Logger::LogLevel level = stringToLevel(config->as<std::string>((*i) + "/level"));

        boost::shared_ptr<Logger> logger;
        if (type == "file") {
            logger.reset(new FileLogger(level, config, *i));
        }
        else {
            // fallback to syslog
            logger.reset(new SyslogLogger(level, config, *i));
        }

        if (config->as<std::string>((*i) + "/print-thread-id", "") == "yes") {
            logger->setFlag(FLAG_PRINT_THREAD_ID);
        }
        if (config->as<std::string>((*i) + "/print-request-id", "") == "yes") {
            logger->setFlag(FLAG_PRINT_REQUEST_ID);
        }

        loggers_[id] = logger;
        if ((defaultLogger_ == 0) || (id == "default")) {
            defaultLogger_ = logger.get();
        }
    }

    //Legacy config processing
    if (!defaultLogger_) {
        Logger::LogLevel level = stringToLevel(config->as<std::string>("/xscript/logger/level"));
        boost::shared_ptr<Logger> logger(new SyslogLogger(level, config, "/xscript/logger"));
        loggers_["default"] = logger;
        defaultLogger_ = logger.get();
    }


    assert(defaultLogger_);
}

Logger*
LoggerFactory::getLogger(const std::string &id) const {
    LoggerMap::const_iterator l = loggers_.find(id);
    return l == loggers_.end() ? defaultLogger_ : l->second.get();
}

Logger::LogLevel
LoggerFactory::stringToLevel(const std::string& level) {
    if (strncasecmp(level.c_str(), "crit", sizeof("crit")) == 0) {
        return Logger::LEVEL_CRIT;
    }
    else if (strncasecmp(level.c_str(), "error", sizeof("error")) == 0) {
        return Logger::LEVEL_ERROR;
    }
    else if (strncasecmp(level.c_str(), "warn", sizeof("warn")) == 0) {
        return Logger::LEVEL_WARN;
    }
    else if (strncasecmp(level.c_str(), "debug", sizeof("debug")) == 0) {
        return Logger::LEVEL_DEBUG;
    }
    else {
        return Logger::LEVEL_INFO;
    }
}

Logger*
LoggerFactory::getDefaultLogger() const {
    return defaultLogger_;
}

void
LoggerFactory::logRotate() const {
    for (LoggerMap::const_iterator l = loggers_.begin(); l != loggers_.end(); ++l) {
        l->second->logRotate();
    }
    getDefaultLogger()->info("Log rotated");
}

bool
LoggerFactory::wrapFormat(const char *fmt, unsigned char flags, std::string &fmt_new) {
    assert(flags);

    char thread_buf[40];
    char request_buf[50];

    int thread_buf_size = 0;
    int request_buf_size = 0;

    size_t fmt_len = strlen(fmt);

    if (flags & FLAG_PRINT_REQUEST_ID) {
        const Request *req = VirtualHostData::instance()->get();
        if (NULL != req) {
            boost::uint64_t request_id = req->requestID();
            if (request_id) {
                request_buf_size = snprintf(request_buf, sizeof(request_buf), "[requestID: %llu] ", request_id);
            }
        }
    }

    if (flags & FLAG_PRINT_THREAD_ID) {
        thread_buf_size = snprintf(thread_buf, sizeof(thread_buf), "[thread: %lu] ", (unsigned long) pthread_self());
    }

    if (!thread_buf_size && !request_buf_size) {
        return false;
    }

    std::string res;
    res.reserve(fmt_len + thread_buf_size + request_buf_size);
    res.append(request_buf, request_buf_size).append(thread_buf, thread_buf_size).append(fmt, fmt_len);
    res.swap(fmt_new);
    return true;
}

static ComponentRegisterer<LoggerFactory> reg;

} // namespace xscript
