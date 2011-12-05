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
    : param_(param), urlencoding_(true), allow_empty_(false), converted_(false), multi_request_arg_(false)
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

        if (cp->multi()) {
            multi_request_arg_ = !strcasecmp(cp->type(), "requestarg");
        }
    }
}

const RequestFiles*
QueryParamData::files(const Context *ctx) const {
    if (!ctx) {
        return NULL;
    }
    if (!multi_request_arg_) {
        return NULL;
    }

    const Request *req = ctx->request();
    const Param *p = param();
    const std::string &key = p->value();

    return req->getFiles(key.empty() ? p->id() : key);
}

std::string
QueryParamData::asString(const Context *ctx, bool multipart) const {

    if (!ctx) {
        return StringUtils::EMPTY_STRING;
    }

    const Param *p = param();

    std::vector<std::string> values;
    if (multi_request_arg_) {
        const Request *req = ctx->request();
        const std::string &key = p->value();
        req->getArg(key.empty() ? p->id() : key, values);
        if (values.empty()) {
            return StringUtils::EMPTY_STRING;
        }
    }
    else {
        std::string val = param_->asString(ctx);
        if (val.empty()) {
            return StringUtils::EMPTY_STRING;
        }
        values.push_back(StringUtils::EMPTY_STRING);
        values.front().swap(val);
    }

    std::auto_ptr<Encoder> encoder;
    if (!encoding_.empty()) {
        encoder = Encoder::createEscaping("utf-8", encoding_.c_str());
    }
    std::string result = !encoder.get() ? values.front() : encoder->encode(values.front());

    if (urlencoding_ && !multipart) {
        result = StringUtils::urlencode(result);
    }

    size_t sz = values.size();
    if (sz > 1) {
	result.reserve(result.size() + 100 * sz);
        const std::string &id = p->id();
        for (size_t i = 1; i < sz; ++i) {
            std::string val = (NULL == encoder.get() ? values[i] : encoder->encode(values[i]));
            if (urlencoding_) {
                val = StringUtils::urlencode(val);
            }
            result.push_back('&');
            result.append(id);
            result.push_back('=');
            result.append(val);
        }
    }
    return result;
}

void
QueryParamData::add(const Context *ctx, QueryParamsArgList &al) const {

    if (al.multipart && multi_request_arg_) {
        al.add(StringUtils::EMPTY_STRING);
        return;
    }

    std::string value = asString(ctx, al.multipart);
    if (value.empty() && !allow_empty_) {
        al.add(value);
    }
    else if (converted_) {
        const ConvertedParam *cp = static_cast<const ConvertedParam*>(param_.get());
        al.addAs(cp->as(), value);
    }
    else {
        al.add(value);
    }
}

std::string
QueryParamData::info() const {

    const Param *p = param();
    const std::string &value = p->value();
    const std::string &id = p->id();

    std::string str;
    str.reserve(id.size() + value.size() + 30);

    str.push_back(' ');
    str.append(id);
    str.push_back(':');
    str.append(p->type());
    if (!value.empty()) {
        str.push_back('(');
        str.append(value);
        str.push_back(')');
    }
    return str;
}

std::string
QueryParamData::queryStringValue(const QueryParamsArgList &al, unsigned int index) const {
    const std::string &v = al.at(index);
    if (v.empty()) {
        if (allowEmpty()) {
            return param()->id();
        }
        return StringUtils::EMPTY_STRING;
    }

    const std::string &param_id = param()->id();

    std::string res;
    res.reserve(param_id.size() + v.size() + 1);
    res.assign(param_id);
    res.push_back('=');
    res.append(v);
    return res;
}

static std::string
createMultipartString(const std::string &param_id, const std::string &value, const std::string &boundary) {
    std::string res;
    res.reserve(param_id.size() + value.size() + 60);
    res.append(boundary).append("\r\nContent-Disposition: form-data; name=\"");
    res.append(param_id).append("\"\r\n\r\n", 5).append(value).append("\r\n", 2);
    return res;
}

static std::string
createMultipartFile(const std::string &param_id, const RequestFile &file, const std::string &boundary) {
    std::pair<const char*, std::streamsize> data = file.data();
    std::string res;
    res.reserve(param_id.size() + data.second + file.remoteName().size() + file.type().size() + 100);
    res.append(boundary).append("\r\nContent-Disposition: form-data; name=\"");
    res.append(param_id).append("\"; filename=\"").append(file.remoteName()).append("\"\r\n", 3);
    res.append("Content-Type: ").append(file.type()).append("\r\n\r\n", 4);
    res.append(data.first, data.second).append("\r\n", 2);
    return res;
}

std::string
QueryParamData::multipartValue(
    const QueryParamsArgList &al, unsigned int index,
    const std::string &boundary, const Request &req) const {

    const Param *p = param();
    const std::string &param_id = p->id();
    if (!multi_request_arg_) {
        const std::string &v = al.at(index);
        if (v.empty()) {
            if (!allowEmpty()) {
                return StringUtils::EMPTY_STRING;
            }
        }
        return createMultipartString(param_id, v, boundary);
    }

    std::string res;
    std::vector<std::string> values;
    const std::string &key = p->value();
    req.getArg(key.empty() ? param_id : key, values);
    if (values.size() != 1 || allowEmpty() || !values.front().empty()) {
        for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it) {
            res.append(createMultipartString(param_id, *it, boundary));
        }    
    }

    const RequestFiles *files = req.getFiles(key.empty() ? param_id : key);
    if (NULL != files) {
        for (RequestFiles::const_iterator it = files->begin(); it != files->end(); ++it) {
            res.append(createMultipartFile(param_id, *it, boundary));
        }    
    }
    return res;
}

} // namespace xscript
