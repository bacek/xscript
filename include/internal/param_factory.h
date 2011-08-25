#ifndef _XSCRIPT_DETAILS_PARAM_FACTORY_H_
#define _XSCRIPT_DETAILS_PARAM_FACTORY_H_

#include <string>
#include <memory>
#include <boost/utility.hpp>
#include <libxml/tree.h>

#include "xscript/param.h"

#include "internal/hash.h"
#include "internal/hashmap.h"

#ifndef HAVE_HASHMAP
#include <map>
#endif

namespace xscript {

class Param;
class Object;

#ifndef HAVE_HASHMAP
typedef std::map<std::string, ParamCreator> CreatorMap;
#else
typedef details::hash_map<std::string, ParamCreator, details::StringHash> CreatorMap;
#endif

class ParamFactory : private boost::noncopyable {
public:
    ParamFactory();
    virtual ~ParamFactory();

    static ParamFactory* instance();
    
    void registerCreator(const char *name, ParamCreator c);
    std::auto_ptr<Param> param(Object *owner, xmlNodePtr node, bool visit_properties, const char *default_type);

private:
    friend class std::auto_ptr<ParamFactory>;
    ParamCreator findCreator(const std::string &name) const;

private:
    CreatorMap creators_;
};

} // namespace xscript

#endif // _XSCRIPT_DETAILS_PARAM_FACTORY_H_
