#ifndef _XSCRIPT_STANDARD_REGEX_VALIDATOR_H_
#define _XSCRIPT_STANDARD_REGEX_VALIDATOR_H_

#include <pcre.h>
#include "xscript/validator.h"

namespace xscript
{

/**
 * Regex validator. Validate param using regexes.
 * Example of usage:
 *   param type="QueryArg" id="foo" validator="regex" pattern="^\S+$"
 */
class RegexValidator : public Validator {
public:
    RegexValidator(xmlNodePtr node);
    ~RegexValidator();
    
    static Validator* create(xmlNodePtr node) {
        return (Validator*)new RangeValidator(node);
    }
protected:
    virtual bool isPassed(const Context *ctx, const Param &value) const;
private:
    pcre* re_;    ///<! Compiled RE
};



} // namespace xscript

#endif
