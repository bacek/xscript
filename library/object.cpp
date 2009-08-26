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
#include "xscript/vhost_data.h"
#include "xscript/xml_util.h"

#include "internal/param_factory.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

Object::Object() {
}

Object::~Object() {
    std::for_each(params_.begin(), params_.end(), boost::checked_deleter<Param>());
}

void
Object::postParse() {
}

void
Object::xsltName(const std::string &value) {
    if (value.empty()) {
        xslt_name_.erase();
        return;
    }

    if (value[0] == '/') {
        std::string full_name = VirtualHostData::instance()->getDocumentRoot(NULL) + value;
        if (FileUtils::fileExists(full_name)) {
            xslt_name_ = fullName(full_name);
            return;
        }
        log()->warn("absolute path in stylesheet href: %s", value.c_str());
    }

    xslt_name_ = fullName(value);
}

void
Object::checkParam(const Param *param) const {
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
}

bool
Object::xsltParamNode(const xmlNodePtr node) const {
    return node->name && xmlStrncasecmp(node->name, (const xmlChar*) "xslt-param", sizeof("xslt-param")) == 0;
}

void
Object::parseXsltParamNode(const xmlNodePtr node, ParamFactory *pf) {
    std::auto_ptr<Param> p = pf->param(this, node);
    log()->debug("%s, creating param %s", BOOST_CURRENT_FUNCTION, p->id().c_str());
    checkParam(p.get());
    params_.push_back(p.get());
    p.release();
}

void
Object::applyStylesheet(boost::shared_ptr<Stylesheet> sh, boost::shared_ptr<Context> ctx, XmlDocHelper &doc, bool need_copy) {

    assert(NULL != doc.get());
    if (need_copy) {
        XmlDocHelper newdoc = sh->apply(this, ctx, doc);
        doc = XmlDocHelper(xmlCopyDoc(newdoc.get(), 1));
    }
    else {
        doc = sh->apply(this, ctx, doc);
    }
    XmlUtils::throwUnless(NULL != doc.get());
}

std::string
Object::createTagKey(const Context *ctx) const {
    (void)ctx;
    return StringUtils::EMPTY_STRING;
}

} // namespace xscript
