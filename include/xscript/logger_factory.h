#ifndef _XSCRIPT_LOGGER_FACTORY_H_
#define _XSCRIPT_LOGGER_FACTORY_H_

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>
#include <xscript/component.h>
#include <xscript/control_extension.h>
#include <xscript/logger.h>
#include <xscript/block.h>

namespace xscript {

class LoggerFactory : public Component<LoggerFactory> {
public:
    LoggerFactory();
    virtual ~LoggerFactory();

    friend class ComponentRegisterer<LoggerFactory>;
    

    virtual void init(const Config *config);

    /**
     * Get logger by id. If logger not found returns default one.
     */
    Logger* getLogger(const std::string &id) const;

    /**
     * Get default logger
     */
    Logger* getDefaultLogger() const;

    /**
     */
    std::auto_ptr<Block> createBlock(const ControlExtension *ext, Xml *owner, xmlNodePtr node);

    /**
     * Enforce log rotating.
     */
    void logRotate() const;

private:
    // Get level from string. Fallback to LEVEL_CRIT if passed something wrong.
    Logger::LogLevel stringToLevel(const std::string& level);

    Logger *defaultLogger_;

    typedef std::map<std::string, boost::shared_ptr<Logger> > LoggerMap;
    LoggerMap loggers_;

};

} // namespace xscript

#endif // _XSCRIPT_LOGGER_FACTORY_H_
