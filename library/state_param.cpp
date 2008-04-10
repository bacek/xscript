#include "settings.h"

#include <sstream>
#include <stdexcept>

#include "xscript/args.h"
#include "xscript/param.h"
#include "xscript/state.h"
#include "xscript/context.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class StateParam : public Param
{
public:
	StateParam(Object *owner, xmlNodePtr node);
	virtual ~StateParam();
	
	virtual std::string asString(const Context *ctx) const;
	virtual void add(const Context *ctx, ArgList &al) const;

	static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);
};

StateParam::StateParam(Object *owner, xmlNodePtr node) :
	Param(owner, node)
{
}

StateParam::~StateParam() {
}
	
std::string
StateParam::asString(const Context *ctx) const {
	
	std::stringstream stream;
	std::map<std::string, StateValue> vals;
	
	boost::shared_ptr<State> state = ctx->state();
	state->values(vals);

	for (std::map<std::string, StateValue>::iterator i = vals.begin(), end = vals.end(); i != end; ++i) {
		stream << i->first << ":" << i->second.value() << std::endl;
	}
	return stream.str();
}

void
StateParam::add(const Context *ctx, ArgList &al) const {
	al.addState(ctx);
}

std::auto_ptr<Param>
StateParam::create(Object *owner, xmlNodePtr node) {
	return std::auto_ptr<Param>(new StateParam(owner, node));
}

static CreatorRegisterer reg_("state", &StateParam::create);

} // namespace xscript
