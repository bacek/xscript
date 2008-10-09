#include "regex_validator.h"
#include "xscript/param.h"

namespace xscript
{

RegexValidator::RegexValidator(xmlNodePtr node)
    : Validator(node), re_(NULL)
{
}

RegexValidator::~RegexValidator()
{
    // Free compiled re
    if (re_) pcre_free(re_);
}

bool 
RegexValidator::isPassed(const Context *ctx, const Param &value) const
{
    return true;
}

Validator* RegexValidator::create(xmlNodePtr node) {
    return (Validator*)new RegexValidator(node);
}

namespace {
static ValidatorRegisterer reg("regex", &RegexValidator::create);
}

}

