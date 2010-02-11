#ifndef _XSCRIPT_TEST_UTILS_H_
#define _XSCRIPT_TEST_UTILS_H_

#include <sstream>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "xscript/context.h"
#include "xscript/request.h"
#include "xscript/response.h"
#include "xscript/script.h"
#include "xscript/script_factory.h"
#include "xscript/state.h"

namespace xscript {
namespace TestUtils {

static boost::shared_ptr<Context>
createEnv(const std::string &script_name, char *env[]) {
    boost::shared_ptr<Request> request(new Request());
    boost::shared_ptr<Response> response(new Response());   
    boost::shared_ptr<State> state(new State());
    
    if (env) {
        std::stringstream stream(StringUtils::EMPTY_STRING);  
        request->attach(&stream, env);
    }
    
    boost::shared_ptr<Script> script = ScriptFactory::createScript(script_name);
    return boost::shared_ptr<Context>(
            new Context(script, state, request, response));
}

static boost::shared_ptr<Context>
createEnv(const std::string &script_name) {
    return createEnv(script_name, NULL);
}

} // namespace TestUtils
} // namespace xscript

#endif // _XSCRIPT_TEST_UTILS_H_
