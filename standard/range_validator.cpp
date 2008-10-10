#include "range_validator.h"

namespace xscript
{

static ValidatorRegisterer r1("int_range", &RangeValidatorBase<int>::create);
static ValidatorRegisterer r2("long_range", &RangeValidatorBase<long>::create);
static ValidatorRegisterer r3("double_range", &RangeValidatorBase<double>::create);

};
