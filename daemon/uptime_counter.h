#ifndef _XSCRIPT_DAEMON_UPTIME_COUNTER_H_
#define _XSCRIPT_DAEMON_UPTIME_COUNTER_H_

#include <time.h>
#include "xscript/counter_base.h"

namespace xscript {
/**
 * Uptime counter for fcgi daemon
 */
class UptimeCounter : public CounterBase {
public:
    UptimeCounter();

    XmlNodeHelper createReport() const;

private:
    time_t startTime_;

};
}

#endif
