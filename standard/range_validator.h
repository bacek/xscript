#ifndef _XSCRIPT_STANDARD_RANGE_VALIDATOR_H_
#define _XSCRIPT_STANDARD_RANGE_VALIDATOR_H_

#include <boost/lexical_cast.hpp>
#include "xscript/param.h"
#include "xscript/xml_util.h"
#include "xscript/validator.h"

namespace xscript
{

/**
 * Range validator. Validate that numeric value of param within given range.
 * Example of usage:
 *   param type="QueryArg" id="foo" validator="int_range" min="1" max="42"
 *
 * At least min or max should be specified.
 *
 * TODO: We really need unit tests...
 */
template<typename T>
class RangeValidator : public Validator {
public:
    RangeValidator(xmlNodePtr node)
	: Validator(node), has_min_(false), has_max_(false) {
        xmlAttrPtr min = xmlHasProp(node, (const xmlChar*)"min");
        if (min) {
            has_min_ = true;
            min_ = boost::lexical_cast<T>(XmlUtils::value(min));
            xmlRemoveProp(min); // libxml will free memory
        }

        xmlAttrPtr max = xmlHasProp(node, (const xmlChar*)"max");
        if (max) {
            has_max_ = true;
            max_ = boost::lexical_cast<T>(XmlUtils::value(max));
            xmlRemoveProp(max); // libxml will free memory
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
            return (!has_min_ || (min_ <= val)) && (!has_max_ || (val <= max_));
        }
        catch(...) {
            return false;
        }
    }

private:
    bool has_min_, has_max_;
    T min_, max_;
};


} // namespace xscript

#endif
