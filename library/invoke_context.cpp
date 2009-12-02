#include "settings.h"

#include "xscript/invoke_context.h"
#include "xscript/tag.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {

struct InvokeContext::ContextData {
    ContextData() : tagged_(false), have_cached_copy_(false) {}
    XmlDocSharedHelper doc_;
    bool tagged_;
    Tag tag_;
    ResultType result_type_;
    bool have_cached_copy_;
};

InvokeContext::InvokeContext() : ctx_data_(new ContextData()) {
}

InvokeContext::~InvokeContext() {
    delete ctx_data_;
}

XmlDocSharedHelper
InvokeContext::resultDoc() const {
    return ctx_data_->doc_;
}

InvokeContext::ResultType
InvokeContext::resultType() const {
    return ctx_data_->result_type_;
}

const Tag&
InvokeContext::tag() const {
    return ctx_data_->tag_;
}

bool
InvokeContext::tagged() const {
    return ctx_data_->tagged_;
}

bool
InvokeContext::haveCachedCopy() const {
    return ctx_data_->have_cached_copy_;
}

void
InvokeContext::haveCachedCopy(bool flag) {
    ctx_data_->have_cached_copy_ = flag;
}

void
InvokeContext::resultDoc(const XmlDocSharedHelper &doc) {
    ctx_data_->doc_.reset();
    ctx_data_->doc_ = doc;
}

void
InvokeContext::resultDoc(XmlDocHelper doc) {
    ctx_data_->doc_ = XmlDocSharedHelper(new XmlDocHelper(doc));
}

void
InvokeContext::resultType(ResultType type) {
    ctx_data_->result_type_ = type;
}

void
InvokeContext::tag(const Tag &tag) {
    ctx_data_->tag_ = tag;
    ctx_data_->tagged_ = true;
}

void
InvokeContext::resetTag() {
    ctx_data_->tag_ = Tag();
    ctx_data_->tagged_ = false;
}

bool
InvokeContext::error() const {
    return ERROR == ctx_data_->result_type_;
}
    
bool
InvokeContext::success() const {
    return SUCCESS == ctx_data_->result_type_;
}

} // namespace xscript
