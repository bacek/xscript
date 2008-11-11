#include "settings.h"

#include <map>
#include <boost/cstdint.hpp>
#include "xscript/util.h"
#include "range_validator.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

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

    std::string as = StringUtils::tolower(XmlUtils::value(as_prop));

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
    // NB: key must be lowercase!
    res["float"] = &RangeValidatorBase<float>::create;
    res["double"] = &RangeValidatorBase<double>::create;
    res["long"] = &RangeValidatorBase<boost::int32_t>::create;
    res["ulong"] = &RangeValidatorBase<boost::uint32_t>::create;
    res["longlong"] = &RangeValidatorBase<boost::int64_t>::create;
    res["long long"] = &RangeValidatorBase<boost::int64_t>::create;
    res["ulonglong"] = &RangeValidatorBase<boost::uint64_t>::create;
    res["unsigned long long"] = &RangeValidatorBase<boost::uint64_t>::create;

    return res;
}

static ValidatorRegisterer r0("range", &createRangeValidator);
static ValidatorRegisterer r1("int_range", &RangeValidatorBase<int>::create);
static ValidatorRegisterer r2("long_range", &RangeValidatorBase<long>::create);
static ValidatorRegisterer r3("double_range", &RangeValidatorBase<double>::create);

};
