#ifndef _XSCRIPT_STANDARD_RANGE_VALIDATOR_H_
#define _XSCRIPT_STANDARD_RANGE_VALIDATOR_H_

#include <boost/lexical_cast.hpp>
#include "xscript/param.h"
#include "xscript/xml_util.h"
#include "xscript/validator.h"

#include <iostream>

namespace xscript
{

/**
 * Range validator. Validate that numeric value of param within given range.
 * Range is tested to include "mix", but not "max" similar to classic C++ style
 * [min, max) checks.
 *
 * Example of usage:
 *   param type="QueryArg" id="foo" validator="int_range" min="1" max="42"
 *
 * At least min or max should be specified.
 *
 */
template<typename T>
class RangeValidatorBase : public Validator {
public:
    RangeValidatorBase(xmlNodePtr node)
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

        if (has_min_ && has_max_ && (min_ > max_)) {
            throw std::runtime_error("Invalid range");
        }
    }
    ~RangeValidatorBase() {}

    static Validator* create(xmlNodePtr node) {
        return (Validator*)new RangeValidatorBase(node);
    }

    /// Check "string" for range. Try to cast to proper type and check.
    /// Made public for testing purpose only...
    bool checkString(const std::string &value) const {
        try {
            T val = boost::lexical_cast<T>(value);
            return (!has_min_ || (min_ <= val)) && (!has_max_ || (val < max_));
        }
        catch(...) {
            return false;
        }
    }

protected:
    /// Check. Return true if validator passed.
    virtual bool isPassed(const Context *ctx, const Param &value) const {
        return checkString(value.asString(ctx));
    }

private:
    bool has_min_, has_max_;
    T min_, max_;
};


} // namespace xscript

#endif
