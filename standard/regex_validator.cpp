#include <iostream>
#include "regex_validator.h"
#include "xscript/param.h"
#include "xscript/xml_util.h"

namespace xscript
{

RegexValidator::RegexValidator(xmlNodePtr node)
    : Validator(node), re_(NULL), pcre_options_(PCRE_UTF8)
{
    // Fetch mandatory pattern
    xmlAttrPtr pattern_attr = xmlHasProp(node, (const xmlChar*)"pattern");
    if (!pattern_attr)
        throw std::runtime_error("Pattern not provided");
    
    std::string pattern = XmlUtils::value(pattern_attr);
    xmlRemoveProp(pattern_attr); // libxml will free memory

    // Sanity check: pattern shouldn't be empty
    if (pattern.empty())
        throw std::runtime_error("Empty pattern in regex validator");
  
    // Check supported options.
    xmlAttrPtr options_attr = xmlHasProp(node, (const xmlChar*)"options");
    if (options_attr) {
        std::string options = XmlUtils::value(options_attr);
        xmlRemoveProp(options_attr);

        const char* c = options.c_str();
        while (*c) {
            switch (*c) {
                case 'i':
                    pcre_options_ |= PCRE_CASELESS;
                    break;

                default:
                    throw std::runtime_error(std::string("Unknown regex option: ") + *c);
            }

            ++c;
        }
    }


    // Time to compile regex
    const char* compile_error;
    int error_offset;
    re_ = pcre_compile(pattern.c_str(), pcre_options_,
                      &compile_error, &error_offset, NULL);
  
    if (re_ == NULL)
        throw std::runtime_error(std::string("Regex compilation failed: ") + compile_error + " at " + boost::lexical_cast<std::string>(error_offset));
    
    bzero((void*)&re_extra_, sizeof(re_extra_));

}

RegexValidator::~RegexValidator()
{
    // Free compiled re
    if (re_) pcre_free(re_);
}

bool 
RegexValidator::isPassed(const Context *ctx, const Param &value) const
{
    return checkString(value.asString(ctx));
}

bool 
RegexValidator::checkString(const std::string &value) const
{
    int rc = pcre_exec(re_, &re_extra_, value.c_str(), value.length(), 0, 0, NULL, 0);
    //std::cerr << "rc: " << rc << " options: " << pcre_options_ << std::endl;
    return rc != -1;
}

Validator* RegexValidator::create(xmlNodePtr node) {
    return (Validator*)new RegexValidator(node);
}

namespace {
static ValidatorRegisterer reg("regex", &RegexValidator::create);
}

}

