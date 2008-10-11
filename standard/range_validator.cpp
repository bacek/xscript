#include <map>
#include "range_validator.h"

namespace xscript
{


// Map string->range_constructor
typedef std::map<std::string, ValidatorConstructor> Constructors;

Constructors createConstructors();

static const Constructors constructors = createConstructors();

Validator * 
createRangeValidator(xmlNodePtr node) {
    xmlAttrPtr as_prop = xmlHasProp(node, (const xmlChar*)"as");
    if (!as_prop)
        throw std::runtime_error("Can't create range without type");

    std::string as = XmlUtils::value(as_prop);

    Constructors::const_iterator c = constructors.find(as);
    if (c == constructors.end()) {
        throw std::runtime_error("Can't create range for unknown type: " + as);
    }
    else {
        return c->second(node);
    }
}

Constructors
createConstructors() {
    Constructors res;
    res["int"] = &RangeValidatorBase<int>::create;
    res["Int"] = &RangeValidatorBase<int>::create;
    res["long"] = &RangeValidatorBase<long>::create;
    res["Long"] = &RangeValidatorBase<long>::create;
    res["double"] = &RangeValidatorBase<double>::create;
    res["Double"] = &RangeValidatorBase<double>::create;

    return res;
}

static ValidatorRegisterer r0("range", &createRangeValidator);
static ValidatorRegisterer r1("int_range", &RangeValidatorBase<int>::create);
static ValidatorRegisterer r2("long_range", &RangeValidatorBase<long>::create);
static ValidatorRegisterer r3("double_range", &RangeValidatorBase<double>::create);

};
