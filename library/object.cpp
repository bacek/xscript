#include "settings.h"

#include <algorithm>
#include <stdexcept>

#include <boost/checked_delete.hpp>
#include <boost/current_function.hpp>

#include "xscript/context.h"
#include "xscript/exception.h"
#include "xscript/logger.h"
#include "xscript/object.h"
#include "xscript/param.h"
#include "xscript/stylesheet.h"
#include "xscript/util.h"
#include "xscript/vhost_data.h"
#include "xscript/xml_util.h"

#include "internal/block_helpers.h"
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
    
    DynamicParam xslt_name_;
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
}

std::string
Object::xsltName(const Context *ctx) const {
    std::string xslt = data_->xslt_name_.value(ctx);
    if (!data_->xslt_name_.fromState()) {
        return xslt;
    }
    return xslt.empty() ? xslt : fullName(xslt);
}

const std::vector<Param*>&
Object::xsltParams() const {
    return data_->params_;
}

void
Object::postParse() {
}

void
Object::xsltName(const char *value, const char *type) {
    if (NULL != type) {
        data_->xslt_name_.assign(value, type);
        return;
    }

    if (NULL == value || '\0' == value[0]) {
        data_->xslt_name_.assign(value, NULL);
        return;
    }

    std::string xslt(value);
    if (value[0] == '/') {
        std::string full_name = VirtualHostData::instance()->getDocumentRoot(NULL) + value;
        if (FileUtils::fileExists(full_name)) {
            xslt = full_name;
        }
        log()->warn("absolute path in stylesheet href: %s", value);
    }

    data_->xslt_name_.assign(fullName(xslt).c_str(), NULL);
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
                        boost::shared_ptr<InvokeContext> invoke_ctx,
                        XmlDocSharedHelper &doc,
                        bool need_copy) {
    assert(NULL != doc.get());
    XmlDocHelper newdoc = sh->apply(this, ctx, invoke_ctx, doc.get());
    ctx->addDoc(doc);

    if (need_copy) {
        doc.reset(xmlCopyDoc(newdoc.get(), 1));
        ctx->addDoc(newdoc);
    }
    else {
        doc.reset(newdoc.release());
    }
    XmlUtils::throwUnless(NULL != doc.get());
}

std::auto_ptr<Param>
Object::createParam(const xmlNodePtr node, const char *default_type) {
    ParamFactory *pf = ParamFactory::instance();
    return pf->param(this, node, true, default_type);
}

std::auto_ptr<Param>
Object::createUncheckedParam(const xmlNodePtr node, const char *default_type) {
    ParamFactory *pf = ParamFactory::instance();
    return pf->param(this, node, false, default_type);
}

const std::string&
Object::xsltNameRaw() const {
    return data_->xslt_name_.value();
}

bool
Object::xsltDefined() const {
    return !xsltNameRaw().empty();
}

} // namespace xscript
