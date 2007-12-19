#include "settings.h"

#include <cerrno>
#include <fstream>
#include <sstream>
#include <boost/crc.hpp>
#include <boost/cstdint.hpp>

#include <sys/stat.h>
#include <libxml/xmlIO.h>

#include "xscript/tag.h"
#include "xscript/util.h"
#include "xscript/param.h"
#include "xscript/tagged_block.h"
#include "xscript/logger.h"
#include "xscript/config.h"
#include "xscript/context.h"
#include "xscript/tagged_cache.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

extern "C" int closeFunc(void *ctx);
extern "C" int writeFunc(void *ctx, const char *buffer, int len);

class TaggedKeyDisk : public TagKey
{
public:
	TaggedKeyDisk(const Context *ctx, const TaggedBlock *block);
	
	boost::uint32_t number() const;
	virtual const std::string& asString() const;
	const std::string& filename() const;

private:
	std::string value_;
	std::string filename_;
	boost::uint32_t number_;
};

class TaggedCacheDisk : public TaggedCache
{
public:
	TaggedCacheDisk();
	virtual ~TaggedCacheDisk();

	virtual void init(const Config *config);

	virtual time_t minimalCacheTime() const;
	
	virtual bool loadDoc(const TagKey *key, Tag &tag, XmlDocHelper &doc);
	virtual bool saveDoc(const TagKey *key, const Tag &tag, const XmlDocHelper &doc);
	virtual std::auto_ptr<TagKey> createKey(const Context *ctx, const TaggedBlock *block) const;

private:
	
	static void makeDir(const std::string &name);
	static void createDir(const std::string &name);
	
	static bool load(const std::string &path, const std::string &key, Tag &tag, std::vector<char> &doc_data);
	static bool save(const std::string &path, const std::string &key, const Tag &tag, const std::string &doc);


	static const time_t DEFAULT_CACHE_TIME;
	static const boost::uint32_t VERSION_SIGNATURE_UNMARKED;
	static const boost::uint32_t VERSION_SIGNATURE_MARKED;

private:
	time_t min_time_;
	std::string root_;
	boost::mutex mutexes_[256];
};

const time_t TaggedCacheDisk::DEFAULT_CACHE_TIME = 5; // sec
const boost::uint32_t TaggedCacheDisk::VERSION_SIGNATURE_UNMARKED = 0xdfc00001;
const boost::uint32_t TaggedCacheDisk::VERSION_SIGNATURE_MARKED = 0xdfc00002;

TaggedKeyDisk::TaggedKeyDisk(const Context *ctx, const TaggedBlock *block) 
{
	assert(NULL != ctx);
	assert(NULL != block);
	
	boost::crc_32_type method_hash, params_hash;
	
	if (!block->xsltName().empty()) {
		value_.assign(block->xsltName());
		value_.push_back('|');
	}
	value_.append(block->canonicalMethod(ctx));

	method_hash.process_bytes(value_.data(), value_.size());
	boost::uint32_t method_sum = method_hash.checksum();
	
	std::string param_str;
	const std::vector<Param*> &params = block->params();
	for (unsigned int i = 0, n = params.size(); i < n; ++i) {
		const std::string &val = params[i]->asString(ctx);
		param_str.append(1, ':').append(val);
	}
	params_hash.process_bytes(param_str.data(), param_str.size());
	boost::uint32_t params_sum = params_hash.checksum();
	
	char buf[255];
	boost::uint32_t common = method_sum ^ params_sum;
	
	number_ = common & 0xFF;
	snprintf(buf, sizeof(buf), "%02x/%02x/%04x%04x", number_, 
		(common & 0xFF00) >> 8, method_sum, params_sum);
	
	filename_.assign(buf);
	value_.append(param_str);
}

boost::uint32_t 
TaggedKeyDisk::number() const {
	return number_;
}

const std::string&
TaggedKeyDisk::asString() const {
	return value_;
}

const std::string&
TaggedKeyDisk::filename() const {
	return filename_;
}


TaggedCacheDisk::TaggedCacheDisk() : 
	min_time_(DEFAULT_CACHE_TIME)
{
}

TaggedCacheDisk::~TaggedCacheDisk() {
}

void
TaggedCacheDisk::init(const Config *config) {
	
	root_ = config->as<std::string>("/xscript/tagged-cache-disk/root-dir", "").append("/");
	min_time_ = config->as<time_t>("/xscript/tagged-cache-disk/min-cache-time", DEFAULT_CACHE_TIME);
	if (min_time_ <= 0) {
		min_time_ = DEFAULT_CACHE_TIME;
	}
}

time_t
TaggedCacheDisk::minimalCacheTime() const {
	return min_time_;
}

bool
TaggedCacheDisk::loadDoc(const TagKey *key, Tag &tag, XmlDocHelper &doc) {
	
	const TaggedKeyDisk *dkey = dynamic_cast<const TaggedKeyDisk*>(key);
	assert(NULL != dkey);

	std::string path(root_);
	path.append(dkey->filename());
	
	std::vector<char> vec;
	try {
		boost::mutex::scoped_lock sl(mutexes_[dkey->number()]);
		if (!load(path, key->asString(), tag, vec)) {
			return false;
		}
	}
	catch (const std::exception &e) {
		log()->error("error while loading doc from cache: %s", e.what());
		return false;
	}

	try {
		doc = XmlDocHelper(xmlParseMemory(&vec[0], vec.size()));
		XmlUtils::throwUnless(NULL != doc.get());
		return true;
	}
	catch (const std::exception &e) {
		log()->error("error while parsing doc from cache: %s", e.what());
		return false;
	}
}

bool
TaggedCacheDisk::saveDoc(const TagKey *key, const Tag &tag, const XmlDocHelper &doc) {
	
	const TaggedKeyDisk *dkey = dynamic_cast<const TaggedKeyDisk*>(key);
	assert(NULL != dkey);

	std::string res;
	xmlOutputBufferPtr buf = xmlOutputBufferCreateIO(writeFunc, closeFunc, &res, NULL);
	XmlUtils::throwUnless(NULL != buf);

	try {
		xmlSaveFormatFileTo(buf, doc.get(), "utf-8", 1);
	}
	catch (const std::exception &e) {
		xmlOutputBufferClose(buf);
		log()->error("error while converting doc to string: %s", e.what());
		return false;
	}

	std::string path(root_);
	path.append(dkey->filename());

	try {
		boost::mutex::scoped_lock sl(mutexes_[dkey->number()]);
		createDir(path);
		return save(path, key->asString(), tag, res);
	}
	catch (const std::exception &e) {
		log()->error("error while saving doc to cache: %s", e.what());
		return false;
	}
}

std::auto_ptr<TagKey>
TaggedCacheDisk::createKey(const Context *ctx, const TaggedBlock *block) const {
	return std::auto_ptr<TagKey>(new TaggedKeyDisk(ctx, block));
}

void
TaggedCacheDisk::makeDir(const std::string &name) {
	int res = mkdir(name.c_str(), 0777);
	if (-1 == res && EEXIST != errno) {
		std::stringstream stream;
		StringUtils::report("failed to create dir: ", errno, stream);
		throw std::runtime_error(stream.str());
	}
}

void
TaggedCacheDisk::createDir(const std::string &path) {

	std::string::size_type pos = static_cast<std::string::size_type>(-1);
	while (true) {
		pos = path.find('/', pos + 1);
		if (std::string::npos == pos) {
			break;
		}
		makeDir(path.substr(0, pos));
	}
}

bool
TaggedCacheDisk::load(const std::string &path, const std::string &key, Tag &tag, std::vector<char> &doc_data) {

	std::fstream is(path.c_str(), std::ios::in | std::ios::out);
	is.exceptions(std::ios::badbit | std::ios::eofbit);
	
	boost::uint32_t ver = 0, doclen = 0, key_size = 0;
	
	is.read((char*) &ver, sizeof(boost::uint32_t));
	if (VERSION_SIGNATURE_MARKED != ver && VERSION_SIGNATURE_UNMARKED != ver) {
		throw std::runtime_error("bad signature");
	}
	is.read((char*) &tag.expire_time, sizeof(time_t));
	if (tag.expired()) {
		log()->info("tag expired");
		return false;
	}
	is.read((char*) &tag.last_modified, sizeof(time_t));

	if (VERSION_SIGNATURE_UNMARKED == ver && tag.needPrefetch()) {
		log()->info("need prefetch doc");
		is.seekg(0, std::ios::beg);
		is.write((const char*) &VERSION_SIGNATURE_MARKED, sizeof(boost::uint32_t));
		return false;
	}

	is.read((char*) &key_size, sizeof(boost::uint32_t));

	std::string key_str;
	key_str.resize(key_size);
	is.read((char*) &key_str[0], key_size);
	if (key != key_str) {
		log()->info("tag key clashes with other one");
		return false;
	}

	is.read((char*) &doclen, sizeof(boost::uint32_t));
	if (0 == doclen) {
		throw std::runtime_error("empty loading doc");
	}
	
	doc_data.resize(doclen);
	is.exceptions(std::ios::badbit);
	is.read(&doc_data[0], doclen);
	if (static_cast<std::streamsize>(doclen) != is.gcount()) {
		throw std::runtime_error("failed to load doc");
	}
	
	return true;
}

bool
TaggedCacheDisk::save(const std::string &path, const std::string &key, const Tag &tag, const std::string &doc_str) {
	
	boost::uint32_t doc_size = doc_str.size();
	assert(0 != doc_size);

	try {
		std::ofstream os(path.c_str(), std::ios::out | std::ios::trunc);
		os.exceptions(std::ios::badbit);

		os.write((const char*) &VERSION_SIGNATURE_UNMARKED, sizeof(boost::uint32_t));
		os.write((const char*) &tag.expire_time, sizeof(time_t));
		os.write((const char*) &tag.last_modified, sizeof(time_t));

		boost::uint32_t key_size = key.size();
		os.write((const char*) &key_size, sizeof(boost::uint32_t));
		os.write(key.data(), key_size);

		os.write((const char*) &doc_size, sizeof(boost::uint32_t));
		os.write(doc_str.data(), doc_size);
		
		return true;
	}
	catch (const std::exception &e) {
		unlink(path.c_str());
		throw;
	}
}

extern "C" int
closeFunc(void *ctx) {
	return 0;
}

extern "C" int
writeFunc(void *ctx, const char *buf, int len) {
	try {
		std::string *str = static_cast<std::string*>(ctx);
		str->append(buf, buf + len);
		return len;
	}
	catch (const std::exception &e) {
		log()->error("error while storing doc in cache: %s", e.what());
	}
	catch (...) {
		log()->error("unknown error while storing doc in cache");
	}
	return -1;
}

static ComponentRegisterer<TaggedCache> reg_(new TaggedCacheDisk());

} // namespace xscript
