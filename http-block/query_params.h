#ifndef _XSCRIPT_QUERY_PARAMS_H_
#define _XSCRIPT_QUERY_PARAMS_H_

#include <string>
#include <list>
#include <memory>

#include <libxml/tree.h>

#include <boost/shared_ptr.hpp>

#include "xscript/args.h"
#include "xscript/request_file.h"

namespace xscript {

class Context;
class Param;
class Request;

class QueryParamsArgList : public CheckedStringArgList {
public:
    explicit QueryParamsArgList(bool checked) : CheckedStringArgList(checked), multipart(false) {}

    bool multipart;
};

class QueryParamData {
public:
    explicit QueryParamData(std::auto_ptr<Param> param);

    bool allowEmpty() const { return allow_empty_; }
    bool multiRequestArg() const { return multi_request_arg_; }

    const RequestFiles* files(const Context *ctx) const;

    const Param* param() const { return param_.get(); }

    void parse(xmlNodePtr node);

    void add(const Context *ctx, QueryParamsArgList &al) const;

    std::string info() const;
    std::string queryStringValue(const QueryParamsArgList &al, unsigned int index) const;
    std::string multipartValue(
        const QueryParamsArgList &al, unsigned int index,
        const std::string &boundary, const Request &req) const;

private:
    std::string asString(const Context *ctx, bool multipart) const;

    boost::shared_ptr<Param> param_;
    std::string encoding_;
    bool urlencoding_;
    bool allow_empty_;
    bool converted_;
    bool multi_request_arg_;
};


typedef std::list<QueryParamData> QueryParams;


} // namespace xscript

#endif // _XSCRIPT_QUERY_PARAMS_H_
