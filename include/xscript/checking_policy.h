#ifndef _XSCRIPT_CHECKING_POLICY_H_
#define _XSCRIPT_CHECKING_POLICY_H_

#include <string>
#include <xscript/component.h>

namespace xscript {

class Request;

class CheckingPolicy : public Component<CheckingPolicy> {
public:
    CheckingPolicy();
    virtual ~CheckingPolicy();
};

} // namespace xscript

#endif //_XSCRIPT_CHECKING_POLICY_H_

