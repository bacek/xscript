#ifndef _XSCRIPT_THREADED_BLOCK_H_
#define _XSCRIPT_THREADED_BLOCK_H_

#include <xscript/block.h>

namespace xscript
{

class ThreadedBlock : public virtual Block
{
public:
	ThreadedBlock(Xml* owner, xmlNodePtr node);
	virtual ~ThreadedBlock();

	int timeout() const;
	int remoteTimeout() const;
	
	void threaded(bool value);
	virtual bool threaded() const;
	
protected:
	virtual void property(const char *name, const char *value);

private:
	ThreadedBlock(const ThreadedBlock &);
	ThreadedBlock& operator = (const ThreadedBlock &);
	
private:
	bool threaded_;
	int timeout_, remote_timeout_;
};

} // namespace xscript

#endif // _XSCRIPT_THREADED_BLOCK_H_
