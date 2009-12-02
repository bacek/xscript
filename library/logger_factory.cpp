#include "settings.h"

#include <iostream>
#include <boost/bind.hpp>
#include <vector>

#include "xscript/logger_factory.h"
#include "xscript/config.h"
#include "xscript/control_extension.h"
#include "xscript/xml_util.h"
#include "details/syslog_logger.h"
#include "details/file_logger.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

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

    XmlDocHelper call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) throw (std::exception) {
        (void)invoke_ctx;
        ControlExtension::setControlFlag(ctx.get());
        LoggerFactory::instance()->logRotate();
        XmlDocHelper doc(xmlNewDoc((const xmlChar*) "1.0"));
        XmlUtils::throwUnless(NULL != doc.get());

        xmlNodePtr node = xmlNewDocNode(doc.get(), NULL, (const xmlChar*) "logrotate", (const xmlChar*) "rotated");
        XmlUtils::throwUnless(NULL != node);

        xmlDocSetRootElement(doc.get(), node);
        
        return doc;
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
    
    Config::addForbiddenKey("/xscript/logger/*");
    
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

        std::string pti = config->as<std::string>((*i) + "/print-thread-id", "");
        logger->printThreadId(pti == "yes");

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

static ComponentRegisterer<LoggerFactory> reg;

} // namespace xscript
