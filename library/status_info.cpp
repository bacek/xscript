#include "settings.h"

#include <boost/bind.hpp>

#include "xscript/status_info.h"
#include "xscript/control_extension.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

StatusInfo::StatusInfo() : 
  statBuilder_("status-info")
{
}


void
StatusInfo::init(const Config *config) {
    (void)config;
    ControlExtensionRegistry::Constructor f = boost::bind(boost::mem_fn(&StatBuilder::createBlock), &statBuilder_, _1, _2, _3);
    ControlExtensionRegistry::registerConstructor("status-info", f);
}

REGISTER_COMPONENT(StatusInfo);

} // namespace xscript
