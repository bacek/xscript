#include <boost/bind.hpp>
#include "xscript/status_info.h"
#include "xscript/control_extension.h"

namespace xscript
{

StatusInfo::StatusInfo() 
	: statBuilder_("status-info")
{
}


void StatusInfo::init(const Config *config) {
	(void)config;
	
	ControlExtensionRegistry::constructor_t f = boost::bind(boost::mem_fn(&StatBuilder::createBlock), &statBuilder_, _1, _2, _3);

    ControlExtensionRegistry::registerConstructor("status-info", f);
}

REGISTER_COMPONENT(StatusInfo);

}
