#include "cookie_methods.h"

extern "C" {
	int luaCookieNew(lua_State *);
    int luaCookieName(lua_State *);
	int luaCookieValue(lua_State *);
	int luaCookieSecure(lua_State *);
	int luaCookieExpires(lua_State *);
	int luaCookiePath(lua_State *);
	int luaCookieDomain(lua_State *);
	int luaCookiePermanent(lua_State *);
}

static const struct luaL_reg cookielib [] = {
      {"new",		luaCookieNew},
      {"name",		luaCookieName},
	  {"value",		luaCookieValue},
	  {"secure",	luaCookieSecure},
	  {"expires",	luaCookieExpires},
	  {"path",		luaCookiePath},
	  {"domain",	luaCookieDomain},
	  {"permananet",luaCookiePermanent},
      {NULL, NULL}
    };
    
namespace xscript
{

const struct luaL_reg * getCookieLib() {
	return cookielib;
}

extern "C" {

int luaCookieNew(lua_State *){
	return 0;
}

int luaCookieName(lua_State *){
	return 0;
}

int luaCookieValue(lua_State *){
	return 0;
}

int luaCookieSecure(lua_State *){
	return 0;
}

int luaCookieExpires(lua_State *){
	return 0;
}

int luaCookiePath(lua_State *){
	return 0;
}

int luaCookieDomain(lua_State *){
	return 0;
}

int luaCookiePermanent(lua_State *){
	return 0;
}


}

}
