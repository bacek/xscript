#ifndef _XSCRIPT_FILE_EXTENSION_H_
#define _XSCRIPT_FILE_EXTENSION_H_


#include <xscript/extension.h>

namespace xscript
{
class FileExtension : public Extension
{
public:

	FileExtension();
	~FileExtension();

	virtual const char* name() const;
	virtual const char* nsref() const;
	
	virtual void initContext(Context *ctx);
	virtual void stopContext(Context *ctx);
	virtual void destroyContext(Context *ctx);
	
	virtual std::auto_ptr<Block> createBlock(Xml *owner, xmlNodePtr node);	
	virtual void init(const Config *config);
};

}

#endif
