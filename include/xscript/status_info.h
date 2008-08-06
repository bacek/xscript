#ifndef _XSCRIPT_STATUS_INFO_H_
#define _XSCRIPT_STATUS_INFO_H_

#include <xscript/component.h>
#include <xscript/stat_builder.h>

namespace xscript {

class Config;

class StatusInfo : public Component<StatusInfo> {
public:
    StatusInfo();

    virtual void init(const Config *config);

    StatBuilder& getStatBuilder() {
        return statBuilder_;
    }
private:
    StatBuilder statBuilder_;
};

} // namespace xscript

#endif // _XSCRIPT_STATUS_INFO_H_
