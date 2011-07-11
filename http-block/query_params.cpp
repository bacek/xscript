#include "settings.h"
#include "query_params.h"

#include <strings.h>

#include <cassert>
#include <memory>

#include <boost/checked_delete.hpp>

#include "xscript/args.h"
#include "xscript/context.h"
#include "xscript/encoder.h"
#include "xscript/param.h"
#include "xscript/request.h"
#include "xscript/string_utils.h"
#include "xscript/xml_util.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif


namespace xscript {

static std::string ALLOW_EMPTY_ERRORMSG_UNKNOW_VALUE("query-param, allow-empty tag: unknown value: ");

QueryParamData::QueryParamData(std::auto_ptr<Param> param)
    : param_(param), urlencoding_(true), allow_empty_(false), converted_(false)
{
    assert(NULL != param_.get());
}

void
QueryParamData::parse(xmlNodePtr node) {

    xmlAttrPtr attr = xmlHasProp(node, (const xmlChar *) "allow-empty");
    if (NULL != attr) {
        const char *attr_value = XmlUtils::value(attr);
        if (attr_value && *attr_value) {
            if (!strcasecmp(attr_value, "yes")) {
                allow_empty_ = true;
            }
            else if (!strcasecmp(attr_value, "no")) {
                allow_empty_ = false;
            }
            else {
                throw std::runtime_error(ALLOW_EMPTY_ERRORMSG_UNKNOW_VALUE + attr_value);
            }
        }
	xmlRemoveProp(attr);
    }

    attr = xmlHasProp(node, (const xmlChar *) "urlencode");
    if (NULL != attr) {
        const char *attr_value = XmlUtils::value(attr);
        if (attr_value && *attr_value) {
            if (!strcasecmp(attr_value, "yes") || !strcasecmp(attr_value, "utf-8")) {
                urlencoding_ = true;
            }
            else if (!strcasecmp(attr_value, "no")) {
                urlencoding_ = false;
            }
            else {
                encoding_.assign(attr_value);
                urlencoding_ = true;
            }
        }
	xmlRemoveProp(attr);
    }

    param_->visitProperties();

    const ConvertedParam *cp = dynamic_cast<const ConvertedParam*>(param_.get());
    if (NULL != cp) {
        converted_ = !cp->as().empty();
    }
}

std::string
QueryParamData::asString(const Context *ctx) const {

    if (!ctx) {
        return StringUtils::EMPTY_STRING;
    }

    std::string val = param_->asString(ctx);
    if (val.empty()) {
        return StringUtils::EMPTY_STRING;
    }

    if (!encoding_.empty()) {
        std::auto_ptr<Encoder> encoder = Encoder::createEscaping("utf-8", encoding_.c_str());
        val = encoder->encode(val);
    }
    if (urlencoding_) {
        val = StringUtils::urlencode(val);
    }
    return val;
}

void
QueryParamData::add(const Context *ctx, ArgList &al) const {
    std::string value = asString(ctx);
    if (converted_) {
        const ConvertedParam *cp = static_cast<const ConvertedParam*>(param_.get());
        al.addAs(cp->as(), value);
    }
    else {
        al.add(value);
    }
}


} // namespace xscript
