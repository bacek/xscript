#include "range_validator.h"

namespace xscript
{

Validator * 
createRangeValidator(xmlNodePtr node) {
    xmlAttrPtr as_prop = xmlHasProp(node, (const xmlChar*)"as");
    if (!as_prop)
        throw std::runtime_error("Can't create range without type");

    std::string as = XmlUtils::value(as_prop);

    if (as == "int") {
        return RangeValidatorBase<int>::create(node);
    }
    else if (as == "double") {
        return RangeValidatorBase<double>::create(node);
    }
    else {
        throw std::runtime_error("Can't create range for unknown type: " + as);
    }
}

static ValidatorRegisterer r0("range", &createRangeValidator);
static ValidatorRegisterer r1("int_range", &RangeValidatorBase<int>::create);
static ValidatorRegisterer r2("long_range", &RangeValidatorBase<long>::create);
static ValidatorRegisterer r3("double_range", &RangeValidatorBase<double>::create);

};
