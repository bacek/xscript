#ifndef _XSCRIPT_THREADED_TAGGED_BLOCK_H_
#define _XSCRIPT_THREADED_TAGGED_BLOCK_H_

#include <xscript/threaded_block.h>
#include <xscript/tagged_block.h>
#include <xscript/xml.h>

namespace xscript
{

class ThreadedTaggedBlock : public ThreadedBlock, public TaggedBlock
{
public:
	ThreadedTaggedBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
	virtual ~ThreadedTaggedBlock();
	
protected:
	virtual void property(const char *name, const char *value);
	virtual void postParse();
};

} // namespace xscript

#endif // _XSCRIPT_THREADED_TAGGED_BLOCK_H_
