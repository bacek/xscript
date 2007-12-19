#include "settings.h"
#include "xscript/xml.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

Xml::Xml()
{
}

Xml::~Xml() {
}

std::string
Xml::fullName(const std::string &object) const {
	const std::string &tmp = name();
	std::string::size_type pos = tmp.rfind('/');
	if (std::string::npos != pos) {
		return tmp.substr(0, pos + 1).append(object);
	}
	return object;
}

} // namespace xscript
