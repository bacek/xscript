#include "settings.h"

#include <map>
#include <string>
#include <vector>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <xscript/functors.h>
#include <xscript/xml_helpers.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class Context;
class MistWorkerMethodRegistrator;
class XsltParamFetcher;

class MistWorker {
public:
    virtual ~MistWorker();

    static std::auto_ptr<MistWorker> create(const std::string &method);
    std::auto_ptr<MistWorker> clone() const;
   
    XmlNodeHelper run(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper run(Context *ctx, const XsltParamFetcher &params) const;
    
    bool isAttachStylesheet() const;
    void attachData(const std::string &data);

private:
    friend class MistWorkerMethodRegistrator;
    
    typedef XmlNodeHelper (MistWorker::*Method)(Context*, const std::vector<std::string>&) const;
    typedef std::map<std::string, Method, StringCILess> MethodMap;
    
    MistWorker(Method method);

    static void registerMethod(const std::string &name, Method method);
    
    XmlNodeHelper setStateLong(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateString(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateDouble(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateLongLong(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateRandom(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateDefined(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateUrlencode(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateUrldecode(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateXmlescape(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateDomain(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateByKeys(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateByDate(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateByQuery(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateByRequest(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateByRequestUrlencoded(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateByHeaders(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateByCookies(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateByProtocol(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateByLocalArgs(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper echoQuery(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper echoRequest(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper echoHeaders(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper echoCookies(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper echoProtocol(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper echoLocalArgs(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateJoinString(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateSplitString(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStateConcatString(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper dropState(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper eraseState(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper dumpState(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper attachStylesheet(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper location(Context *ctx, const std::vector<std::string> &params) const;
    XmlNodeHelper setStatus(Context *ctx, const std::vector<std::string> &params) const;

private:    
    std::string data_;
    Method method_;
    
    static MethodMap methods_;
};

} // namespace xscript
