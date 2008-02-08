#ifndef _XSCRIPT_SANITIZER_H_
#define _XSCRIPT_SANITIZER_H_

#include <string>
#include <xscript/range.h>
#include <xscript/component.h>

namespace xscript
{

class Sanitizer : 
	public virtual Component, 
	public ComponentHolder<Sanitizer>
{
public:
	Sanitizer();
	virtual ~Sanitizer();
	
	virtual void init(const Config *config);
	virtual std::string sanitize(const Range &range);
};

} // namespace xscript

#endif // _XSCRIPT_SANITIZER_H_
