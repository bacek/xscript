#include "settings.h"
#include "state_node.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

StateNode::StateNode() :
	XmlNode("state")
{
}

StateNode::StateNode(const char* type_str, const char* name, const char* content) :
	XmlNode("state")
{
	setType(type_str);
	setName(name);
	setContent(content);
}

void
StateNode::setName(const char* name) {
	setProperty("name", name);
}

} // namespace xscript
