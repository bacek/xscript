#ifndef _XSCRIPT_REFRESHER_H_
#define _XSCRIPT_REFRESHER_H_

#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>

namespace xscript {

class Refresher : private boost::noncopyable {
public:
    Refresher(boost::function<void()> f, boost::uint32_t timeout);
    virtual ~Refresher();
    
private:
    class RefresherData;
    RefresherData *data_;
};

} // namespace xscript

#endif // _XSCRIPT_REFRESHER_H_

