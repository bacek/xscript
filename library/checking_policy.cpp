#include "settings.h"
#include "xscript/checking_policy.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

CheckingPolicy::CheckingPolicy() {
}

CheckingPolicy::~CheckingPolicy() {
}

REGISTER_COMPONENT(CheckingPolicy);

} // namespace xscript
