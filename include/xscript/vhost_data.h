#ifndef _XSCRIPT_VHOST_DATA_H_
#define _XSCRIPT_VHOST_DATA_H_

#include <string>
#include <vector>
#include <boost/utility.hpp>
#include <boost/thread/tss.hpp>
#include <libxml/tree.h>

#include <xscript/component.h>
#include <xscript/request.h>

namespace xscript
{
class VirtualHostData : 
	public virtual Component, 
	public ComponentHolder<VirtualHostData>,
	public ComponentFactory<VirtualHostData>
{
public:
	VirtualHostData();
	virtual ~VirtualHostData();

	virtual void init(const Config *config);
	void set(Request* request);

	virtual bool hasVariable(const Request* request, const std::string& var) const;
	virtual std::string getVariable(const Request* request, const std::string& var) const;
	virtual std::string getKey(const Request* request, const std::string& name) const;
	virtual std::string getOutputEncoding(const Request* request) const;

protected:
	Request* get() const;

private:
	boost::thread_specific_ptr<Request> request_;
};

} // namespace xscript

#endif // _XSCRIPT_VHOST_DATA_H_
