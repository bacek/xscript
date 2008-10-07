#include <boost/lexical_cast.hpp>
#include "xscript/validator.h"
#include "xscript/param.h"

namespace xscript
{

/**
 * Range validator. Validate that numeric value of param within given range.
 * Example of usage:
 *   param type="QueryArg" id="foo" validator="int_range" min="1" max="42"
 *
 * At least min or max should be specified.
 */
template<typename T>
class RangeValidator : public Validator {
public:
    RangeValidator(xmlNodePtr node)
	: Validator(node), has_min_(false), has_max_(false) {
        xmlAttrPtr min = xmlHasProp(node, (const xmlChar*)"min");
        if (min) {
            has_min_ = true;
            min_ = boost::lexical_cast<T>(min_);
        }

        xmlAttrPtr max = xmlHasProp(node, (const xmlChar*)"max");
        if (max) {
            has_max_ = true;
            max_ = boost::lexical_cast<T>(max_);
        }

        if (!(has_min_ || has_max_)) {
            throw std::runtime_error("Insufficient args for range validator");
        }
    }
    ~RangeValidator() {}

    static Validator* create(xmlNodePtr node) {
        return (Validator*)new RangeValidator(node);
    }

protected:
    /// Check. Return true if validator passed.
    virtual bool isPassed(const Context *ctx, const Param &value) const {
        try {
            T val = boost::lexical_cast<T>(value.asString(ctx));
            return (!has_min_ || (min_ <= val)) && (!has_max || (val <= max_));
        }
        catch(...) {
            return false;
        }
    }

private:
    bool has_min_, has_max_;
    T min_, max_;
};


static ValidatorRegisterer r1("int_range", &RangeValidator<int>::create);
static ValidatorRegisterer r1("long_range", &RangeValidator<long>::create);
static ValidatorRegisterer r1("double_range", &RangeValidator<double>::create);

};
