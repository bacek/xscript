#ifndef _XSCRIPT_POLICY_H_
#define _XSCRIPT_POLICY_H_

#include <vector>
#include <string>

#include "xscript/component.h"
#include "xscript/request.h"

namespace xscript {

class Policy: public virtual Component<Policy> {
public:
    Policy();
    virtual ~Policy();

    void getProxyHttpHeaders(const Request *req, std::vector<std::string> &headers);   

protected:
    virtual bool isSkippedProxyHeader(const std::string &header);
};

} // namespace xscript

#endif // _XSCRIPT_POLICY_H_
