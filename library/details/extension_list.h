#ifndef _XSCRIPT_DETAILS_EXTENSION_LIST_H_
#define _XSCRIPT_DETAILS_EXTENSION_LIST_H_

#include "settings.h"

#include <memory>
#include <vector>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>

#include <libxml/tree.h>

#include "xscript/extension.h"
#include "details/phoenix_singleton.h"

namespace xscript
{

class Loader;
class Config;
class Context;

class ExtensionList : 
	private boost::noncopyable,
	public PhoenixSingleton<ExtensionList>
{
public:
	ExtensionList();
	virtual ~ExtensionList();

	inline void initContext(Context *ctx) {
		std::for_each(extensions_.begin(), extensions_.end(), 
			boost::bind(&Extension::initContext, _1, ctx));
	}
	
	inline void stopContext(Context *ctx) {
		std::for_each(extensions_.begin(), extensions_.end(), 
			boost::bind(&Extension::stopContext, _1, ctx));
	}
	
	inline void destroyContext(Context *ctx) {
		std::for_each(extensions_.begin(), extensions_.end(), 
			boost::bind(&Extension::destroyContext, _1, ctx));
	}
	
	inline void init(const Config *config) {
		std::for_each(extensions_.begin(), extensions_.end(), 
			boost::bind(&Extension::init, _1, config));
	}
	
	void registerExtension(ExtensionHelper ext);

	Extension* extension(const xmlNodePtr node) const;
	Extension* extension(const char *name, const char *ref) const;

private:
	friend class std::auto_ptr<ExtensionList>;
	bool accepts(Extension *ext, const char *name, const char *ref) const;

private:
	boost::shared_ptr<Loader> loader_;
	std::vector<Extension*> extensions_;
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_EXTENSION_LIST_H_
