#ifndef _XSCRIPT_SANITIZER_H_
#define _XSCRIPT_SANITIZER_H_

#include <string>
#include <xscript/range.h>
#include <xscript/component.h>

namespace xscript
{

class Sanitizer : public Component<Sanitizer>
{
public:
	Sanitizer();
	virtual ~Sanitizer();
	
	virtual std::string sanitize(const Range &range);
};

} // namespace xscript

#endif // _XSCRIPT_SANITIZER_H_
