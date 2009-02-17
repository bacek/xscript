#include <boost/python.hpp>

#include "../xscript/server_wrapper.h"

BOOST_PYTHON_MODULE(xscript) {
    using namespace boost::python;
    def("initialize", initialize);
    def("renderBuffer", renderBuffer);
    def("renderFile", renderFile);
}


