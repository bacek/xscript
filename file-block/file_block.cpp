#include <sys/stat.h>

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <boost/filesystem/path.hpp>
#include <xscript/logger.h>
#include <xscript/xml.h>
#include <xscript/param.h>
#include <xscript/util.h>
#include <libxml/uri.h>
#include <libxml/xinclude.h>
#include "file_block.h"

namespace xscript
{

namespace fs = boost::filesystem;

FileBlock::FileBlock(const Extension *ext, Xml *owner, xmlNodePtr node) 
	: Block(ext, owner, node), ThreadedBlock(ext, owner, node), TaggedBlock(ext, owner, node), processXInclude_(false)
{
}

FileBlock::~FileBlock() {
}

void
FileBlock::postParse() {
	ThreadedBlock::postParse();
	TaggedBlock::postParse();

	createCanonicalMethod("file.");

	if (method() == "include") {
		processXInclude_ = true;
	}
	else if (method() == "load") {
		processXInclude_ = false;
	}
	else {
		throw std::invalid_argument("Unknown method for file-block: " + method());
	}
}

XmlDocHelper
FileBlock::call(Context *ctx, boost::any &a) throw (std::exception) {
	log()->info("%s, %s", BOOST_CURRENT_FUNCTION, owner()->name().c_str());

	const std::vector<Param*> &p = params();

	if (p.size() < 1 || p.size() > 2) {
		throw std::logic_error("file-block: bad arity");
	}

	std::string file = createFilename(p[0]->asString(ctx));

	if (!tagged()) 
		return loadFile(file);

	struct stat st;
	int res = stat(file.c_str(), &st);
	if (res != 0) {
		// Return empty document if we can't stat file. It's not available anyway.
		return XmlDocHelper();
	}

	const Tag* tag = boost::any_cast<Tag>(&a);

	XmlDocHelper doc;
	bool modified;

	if (tag && tag->last_modified != Tag::UNDEFINED_TIME && st.st_mtime <= tag->last_modified) {
		// We got tag and file modification time not greater than last_modified in tag
		// Set "modified" to false and omit loading doc.
		modified = false;
	}
	else {
		modified = true;
		doc = loadFile(file);
	}

	Tag local_tag(modified, st.st_mtime, Tag::UNDEFINED_TIME);
	a = boost::any(local_tag);

	return doc;
}

XmlDocHelper
FileBlock::loadFile(const std::string& file_name) {
	log()->debug("%s: loading file %s", BOOST_CURRENT_FUNCTION, file_name.c_str());

	XmlDocHelper doc(xmlReadFile(
		file_name.c_str(), 
		NULL,
		XML_PARSE_DTDATTR | XML_PARSE_DTDLOAD | XML_PARSE_NOENT)
	);

	XmlUtils::throwUnless(NULL != doc.get());

	if (processXInclude_) {
		XmlUtils::throwUnless( xmlXIncludeProcessFlags(doc.get(), XML_PARSE_NOENT) >= 0);
	}
	return doc;
}


std::string
FileBlock::createFilename(const std::string& relative_name) {

	if (relative_name.empty()) {
		throw std::runtime_error("Empty relative path in file block");
	}

	fs::path path;
	if (*relative_name.begin() == '/') {
		path = fs::path(relative_name);
	}
	else {
		const std::string &owner_name = owner()->name();
		if (owner_name.empty()) {
			path = fs::path(relative_name);
		}
		else if (*owner_name.rbegin() == '/') {
			path = fs::path(owner_name + relative_name);
		}
		else {
			std::string::size_type pos = owner_name.find_last_of('/');
			if (pos == std::string::npos) {
				path = fs::path(relative_name);
			}
			else {
				path = fs::path(owner_name.substr(0, pos + 1) + relative_name);
			}
		}
	}

	XmlCharHelper canonic_path(xmlCanonicPath((const xmlChar *) path.native_file_string().c_str()));

	return (const char*) canonic_path.get();
}

}

