#ifndef _XSCRIPT_MIST_STATE_PARAM_NODE_H_
#define _XSCRIPT_MIST_STATE_PARAM_NODE_H_

#include <string>
#include <vector>
#include <libxml/tree.h>

namespace xscript
{

class StateParamNode
{
public:
	StateParamNode(xmlNodePtr parent, const char* name);
	virtual ~StateParamNode();

	void createSubNode(const char* val) const;
	void createSubNodes(const std::vector<std::string>& v) const;

	static bool checkName(const char* name);

private:
	StateParamNode(const StateParamNode &);
	StateParamNode& operator = (const StateParamNode &);

private:
	xmlNodePtr parent_;
	const char* name_;
	bool is_valid_name_;
};


} // namespace xscript

#endif // _XSCRIPT_MIST_STATE_PARAM_NODE_H_
