#ifndef _XSCRIPT_MIST_STATE_NODE_H_
#define _XSCRIPT_MIST_STATE_NODE_H_

#include "xml_node.h"

namespace xscript
{

class StateNode : public XmlNode
{
public:
	StateNode();
	StateNode(const char* type_str, const char* name, const char* content);

	void setName(const char* name);

private:
	StateNode(const StateNode &);
	StateNode& operator = (const StateNode &);
};


} // namespace xscript

#endif // _XSCRIPT_MIST_STATE_NODE_H_
