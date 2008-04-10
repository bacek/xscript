#include "settings.h"

#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include "xscript/args.h"
#include "xscript/param.h"
#include "xscript/tagged_block.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

class TagParam : public Param
{
public:
	TagParam(TaggedBlock *owner, xmlNodePtr node);
	virtual ~TagParam();

	virtual void parse();
	virtual std::string asString(const Context *ctx) const;
	virtual void add(const Context *ctx, ArgList &al) const;

	static std::auto_ptr<Param> create(Object *owner, xmlNodePtr node);

private:
	TaggedBlock *owner_;
};

TagParam::TagParam(TaggedBlock *owner, xmlNodePtr node) :
	Param(owner, node), owner_(owner)
{
}

TagParam::~TagParam() {
}

void
TagParam::parse() {
	Param::parse();
	const std::string& v = value();
	if (!v.empty()) {
		owner_->cacheTime(boost::lexical_cast<time_t>(v));
	}
}
	
std::string
TagParam::asString(const Context *ctx) const {
	(void)ctx;
	return std::string();
}

void
TagParam::add(const Context *ctx, ArgList &al) const {
	al.addTag(owner_, ctx);
}

std::auto_ptr<Param>
TagParam::create(Object *owner, xmlNodePtr node) {
	TaggedBlock* tblock = dynamic_cast<TaggedBlock*>(owner);
	if (NULL == tblock) {
		throw std::runtime_error("Conflict: tag param in non-tagged-block");
	}
	tblock->tagged(true);
	return std::auto_ptr<Param>(new TagParam(tblock, node));
}

static CreatorRegisterer reg_("tag", &TagParam::create);

} // namespace xscript
