#include "settings.h"

#include <algorithm>
#include <stdexcept>

#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>

#include "xscript/exception.h"
#include "xscript/logger.h"
#include "xscript/object.h"
#include "xscript/param.h"
#include "xscript/stylesheet.h"
#include "xscript/util.h"
#include "xscript/vhost_data.h"
#include "xscript/xml_util.h"

#include "internal/param_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

class Object::ObjectData {
public:
    ObjectData();
    ~ObjectData();
    
    void checkParam(Param *param);
    
    std::string xslt_name_;
    std::vector<Param*> params_;
};

Object::ObjectData::ObjectData() {
}

Object::ObjectData::~ObjectData() {
    std::for_each(params_.begin(), params_.end(), boost::checked_deleter<Param>());
}

void
Object::ObjectData::checkParam(Param *param) {
    const std::string& id = param->id();
    if (id.empty()) {
        throw std::runtime_error("xsl param without id");
    }

    int size = id.size();
    if (size > 128) {
        throw UnboundRuntimeError(std::string("xsl param with too big size id: ") + id);
    }

    if (!isalpha(id[0]) && id[0] != '_') {
        throw std::runtime_error(std::string("xsl param with incorrect 1 character in id: ") + id);
    }

    for (int i = 1; i < size; ++i) {
        char character = id[i];
        if (isalnum(character) || character == '-' || character == '_') {
            continue;
        }

        throw std::runtime_error(
            std::string("xsl param with incorrect ") +
            boost::lexical_cast<std::string>(i + 1) +
            std::string(" character in id: ") + id);
    }
    
    params_.push_back(param);
}

Object::Object() : data_(new ObjectData()) {
}

Object::~Object() {
    delete data_;
}

const std::string&
Object::xsltName() const {
    return data_->xslt_name_;
}

const std::vector<Param*>&
Object::xsltParams() const {
    return data_->params_;
}

void
Object::postParse() {
}

void
Object::xsltName(const std::string &value) {
    if (value.empty()) {
        data_->xslt_name_.erase();
        return;
    }

    if (value[0] == '/') {
        std::string full_name = VirtualHostData::instance()->getDocumentRoot(NULL) + value;
        if (FileUtils::fileExists(full_name)) {
            data_->xslt_name_ = fullName(full_name);
            return;
        }
        log()->warn("absolute path in stylesheet href: %s", value.c_str());
    }

    data_->xslt_name_ = fullName(value);
}

bool
Object::xsltParamNode(const xmlNodePtr node) const {
    return node->name &&
           xmlStrncasecmp(node->name, (const xmlChar*)"xslt-param", sizeof("xslt-param")) == 0;
}

void
Object::parseXsltParamNode(const xmlNodePtr node) {
    std::auto_ptr<Param> p = createParam(node);
    log()->debug("%s, creating param %s", BOOST_CURRENT_FUNCTION, p->id().c_str());
    data_->checkParam(p.get());
    p.release();
}

void
Object::applyStylesheet(boost::shared_ptr<Stylesheet> sh,
                        boost::shared_ptr<Context> ctx,
                        XmlDocSharedHelper &doc,
                        bool need_copy) {

    assert(NULL != doc->get());
    if (need_copy) {
        XmlDocHelper newdoc = sh->apply(this, ctx, doc->get());
        doc.reset(new XmlDocHelper(xmlCopyDoc(newdoc.get(), 1)));
    }
    else {
        doc.reset(new XmlDocHelper(sh->apply(this, ctx, doc->get())));
    }
    XmlUtils::throwUnless(NULL != doc->get());
}

std::auto_ptr<Param>
Object::createParam(const xmlNodePtr node) {
    ParamFactory *pf = ParamFactory::instance();
    return pf->param(this, node);
}

} // namespace xscript
