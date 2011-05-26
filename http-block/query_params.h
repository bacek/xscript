#ifndef _XSCRIPT_QUERY_PARAMS_H_
#define _XSCRIPT_QUERY_PARAMS_H_

#include <string>
#include <list>
#include <memory>

#include <libxml/tree.h>

#include <boost/shared_ptr.hpp>

namespace xscript {

class ArgList;
class Context;
class Param;

class QueryParamData {
public:
    explicit QueryParamData(std::auto_ptr<Param> param);

    bool allowEmpty() const { return allow_empty_; }
    const Param* param() const { return param_.get(); }

    void parse(xmlNodePtr node);

    void add(const Context *ctx, ArgList &al) const;

private:
    std::string asString(const Context *ctx) const;

    boost::shared_ptr<Param> param_;
    std::string encoding_;
    bool urlencoding_;
    bool allow_empty_;
    bool converted_;
};


typedef std::list<QueryParamData> QueryParams;


} // namespace xscript

#endif // _XSCRIPT_QUERY_PARAMS_H_
