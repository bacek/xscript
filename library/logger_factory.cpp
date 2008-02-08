#include <iostream>
#include <boost/bind.hpp>
#include <vector>
#include "xscript/logger_factory.h"
#include "xscript/config.h"
#include "details/syslog_logger.h"
#include "details/file_logger.h"


namespace xscript
{

LoggerFactory::LoggerFactory() 
    : defaultLogger_(0)
{
}

LoggerFactory::~LoggerFactory() {
}

void LoggerFactory::init(const Config * config) {
	std::vector<std::string> v;
	std::string key("/xscript/logger-factory/logger");
	
	config->subKeys(key, v);
    for(std::vector<std::string>::iterator i = v.begin(), end = v.end(); i != end; ++i) {
		std::string id = config->as<std::string>((*i) + "/id");
        std::string type = config->as<std::string>((*i) + "/type");
        Logger::LogLevel level = stringToLevel(config->as<std::string>((*i) + "/level"));

        boost::shared_ptr<Logger> logger;
        if(type == "file") {
            logger.reset(new FileLogger(level, config, *i));
        }
        else {
            // fallback to syslog 
            logger.reset(new SyslogLogger(level, config, *i));
        }

        loggers_[id] = logger;
        if((defaultLogger_ == 0) || (id == "default")) {
            defaultLogger_ = logger.get();
        }
    }

	//Legacy config processing
	if(!defaultLogger_){
        Logger::LogLevel level = stringToLevel(config->as<std::string>("/xscript/logger/level"));
        boost::shared_ptr<Logger> logger(new SyslogLogger(level, config, "/xscript/logger"));
		loggers_["default"] = logger;
        defaultLogger_ = logger.get();
	}


    assert(defaultLogger_);
}

Logger * LoggerFactory::getLogger(const std::string &id) const {
    loggerMap_t::const_iterator l = loggers_.find(id);
    return l == loggers_.end()
        ? defaultLogger_
        : l->second.get();
}

Logger::LogLevel LoggerFactory::stringToLevel(const std::string& level)
{
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

Logger* LoggerFactory::getDefaultLogger() const
{
    return defaultLogger_;
}

}
