#include "settings.h"

#include <map>
#include <string>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <xscript/functors.h>
#include <xscript/xml_helpers.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class Context;
class Param;
class MistWorkerMethodRegistrator;
class XsltParamFetcher;

class MistWorker {
public:
    virtual ~MistWorker();

    static std::auto_ptr<MistWorker> create(const std::string &method);
   
    XmlNodeHelper run(Context *ctx,
                      const std::vector<Param*> &params,
                      const std::map<unsigned int, std::string> &overrides = EMPTY_OVERRIDES_);
    XmlNodeHelper run(Context *ctx,
                      const XsltParamFetcher &params,
                      const std::map<unsigned int, std::string> &overrides = EMPTY_OVERRIDES_);
    
    const std::string& methodName() const;
    bool isAttachStylesheet() const;

private:
    friend class MistWorkerMethodRegistrator;
    
    typedef boost::function<XmlNodeHelper (Context*, const std::vector<std::string>&)> Method;
    typedef std::map<std::string, Method, StringCILess> MethodMap;
    
    MistWorker(const std::string &method);

    static void registerMethod(const std::string &name, Method method);
    
    static XmlNodeHelper setStateLong(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateString(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateDouble(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateLongLong(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateRandom(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateDefined(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateUrlencode(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateUrldecode(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateXmlescape(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateDomain(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateByKeys(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateByDate(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateByQuery(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateByRequest(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateByRequestUrlencoded(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateByHeaders(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateByCookies(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateByProtocol(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper echoQuery(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper echoRequest(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper echoHeaders(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper echoCookies(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper echoProtocol(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateJoinString(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateSplitString(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStateConcatString(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper dropState(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper dumpState(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper attachStylesheet(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper location(Context *ctx, const std::vector<std::string> &params);
    static XmlNodeHelper setStatus(Context *ctx, const std::vector<std::string> &params);

private:    
    std::string method_name_;
    Method method_;
    
    static MethodMap methods_;
    static std::map<unsigned int, std::string> EMPTY_OVERRIDES_;
};

} // namespace xscript
