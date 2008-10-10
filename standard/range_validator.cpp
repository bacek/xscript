#include "range_validator.h"

namespace xscript
{

static ValidatorRegisterer r1("int_range", &RangeValidator<int>::create);
static ValidatorRegisterer r2("long_range", &RangeValidator<long>::create);
static ValidatorRegisterer r3("double_range", &RangeValidator<double>::create);

};
