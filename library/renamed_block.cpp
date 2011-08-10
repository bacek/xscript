#include "settings.h"
#include <xscript/renamed_block.h>

#include <stdexcept>

#include <xscript/invoke_context.h>
#include <xscript/extension.h>
#include <xscript/xml_helpers.h>

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript {


RenamedBlock::RenamedBlock(const Extension *ext, Xml *owner, xmlNodePtr node) :
        Block(ext, owner, node), root_ns_(NULL) {
}

RenamedBlock::~RenamedBlock() {
}

void
RenamedBlock::property(const char *name, const char *value) {
    if (0 != strncasecmp(name, "name", sizeof("name"))) {
        Block::property(name, value);
        return;
    }
    if (NULL == value || '\0' == value[0]) {
        throw std::runtime_error("Incorrect name attribute");
    }
    std::string root_ns;
    const char* ch = strchr(value, ':');
    if (NULL == ch) {
        root_name_.assign(value);
    }
    else {
        root_ns.assign(value, ch - value);
        if ('\0' == *(ch + 1)) {
            throw std::runtime_error("Incorrect name attribute");
        }
        root_name_.assign(ch + 1);
    }

    if (root_ns.empty()) {
        return;
    }

    const std::map<std::string, std::string> names = namespaces();
    std::map<std::string, std::string>::const_iterator it = names.find(root_ns);
    if (names.end() == it) {
        throw std::runtime_error("Incorrect namespace in name attribute");
    }
    root_ns_ = xmlSearchNsByHref(node()->doc, node(), (const xmlChar*)it->second.c_str());
    if (NULL == root_ns_) {
        throw std::logic_error("Internal error while parsing namespace in name attribute");
    }
}

void
RenamedBlock::wrapInvokeContext(InvokeContext &invoke_ctx) const throw (std::exception) {

    XmlDocSharedHelper ret_doc = invoke_ctx.resultDoc();
    if (!ret_doc.get()) {
        return;
    }
    xmlNodePtr root = xmlDocGetRootElement(ret_doc.get());
    if (!root) {
        return;
    }

    if (root_name_.empty()) {
        xmlNodeSetName(root, (const xmlChar*)extension()->name());
    }
    else {
        xmlNodeSetName(root, (const xmlChar*)root_name_.c_str());
    }
    if (root_ns_) {
        root->nsDef = xmlCopyNamespace(root_ns_);
        xmlSetNs(root, root->nsDef);
    }
}


} // namespace xscript
