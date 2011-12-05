#ifndef _XSCRIPT_INTERNAL_REQUEST_IMPL_H_
#define _XSCRIPT_INTERNAL_REQUEST_IMPL_H_

#include <string>
#include <map>

#include <boost/thread/mutex.hpp>

#include "xscript/request.h"
#include "xscript/types.h"

namespace xscript {

class RequestImpl {
public:
    RequestImpl();
    ~RequestImpl();
    
    friend class Parser;
    friend class Request;
    friend class Request::AttachHandler;
    
private:
    void insertFile(const std::string &name, const std::map<Range, Range, RangeCILess> &m, const Range &content);

    mutable boost::mutex mutex_;
    VarMap vars_;
    CookieMap cookies_;
    std::vector<char> body_;
    HeaderMap headers_;

    std::map<std::string, RequestFiles> files_;
    std::vector<StringUtils::NamedValue> args_;
    bool is_bot_;

    static const std::string HEAD;
    static const std::string HOST_KEY;
    static const std::string CONTENT_TYPE_KEY;
    static const std::string CONTENT_ENCODING_KEY;
    static const std::string CONTENT_LENGTH_KEY;
    
    static const std::string HTTPS_KEY;
    static const std::string SERVER_ADDR_KEY;
    static const std::string SERVER_PORT_KEY;
    
    static const std::string PATH_INFO_KEY;
    static const std::string PATH_TRANSLATED_KEY;
    
    static const std::string SCRIPT_NAME_KEY;
    static const std::string SCRIPT_FILENAME_KEY;
    static const std::string DOCUMENT_ROOT_KEY;
    
    static const std::string REMOTE_USER_KEY;
    static const std::string REMOTE_ADDR_KEY;
    
    static const std::string QUERY_STRING_KEY;
    static const std::string REQUEST_METHOD_KEY;
    static const std::string REQUEST_URI_KEY;
};

} // namespace xscript

#endif // _XSCRIPT_INTERNAL_REQUEST_IMPL_H_
