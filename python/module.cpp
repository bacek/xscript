#include <boost/python.hpp>

#include <pyerrors.h>

#include "../xscript/server_wrapper.h"

namespace xoffline = xscript::offline;

static void
initialize(const char *config_path) {
    std::string error;
    try {
        xoffline::initialize(config_path);
        return;
    }
    catch(const std::exception &e) {
        error = e.what();
    }
    catch(...) {
        error = "Unknown error";
    }
    
    PyErr_SetString(PyExc_RuntimeError, error.c_str());
    boost::python::throw_error_already_set();
    throw std::runtime_error(error);
}

static std::string
renderBuffer(const std::string &url,
             const std::string &xml,
             const std::string &body,
             const std::string &headers,
             const std::string &vars) {
    std::string error;
    try {
        return xoffline::renderBuffer(url, xml, body, headers, vars);
    }
    catch(const std::exception &e) {
        error = e.what();
    }
    catch(...) {
        error = "Unknown error";
    }
    
    PyErr_SetString(PyExc_RuntimeError, error.c_str());
    boost::python::throw_error_already_set();
    throw std::runtime_error(error);
}

static std::string
renderFile(const std::string &file,
           const std::string &body,
           const std::string &headers,
           const std::string &vars) {
    std::string error;
    try {
        return xoffline::renderFile(file, body, headers, vars);
    }
    catch(const std::exception &e) {
        error = e.what();
    }
    catch(...) {
        error = "Unknown error";
    }
    
    PyErr_SetString(PyExc_RuntimeError, error.c_str());
    boost::python::throw_error_already_set();
    throw std::runtime_error(error);
}

BOOST_PYTHON_MODULE(xscript) {
    using namespace boost::python;
    def("initialize", initialize);
    def("renderBuffer", renderBuffer);
    def("renderFile", renderFile);
}

