#ifndef _XSCRIPT_FILE_BLOCK_H_
#define _XSCRIPT_FILE_BLOCK_H_

#include <boost/function.hpp>
#include <xscript/extension.h>
#include <xscript/threaded_tagged_block.h>

namespace xscript {
class FileBlock;

typedef XmlDocHelper (FileBlock::*Method)(const std::string&, Context*);

/**
 * x:file block. Loading local files with <s>jackpot and hookers</s> with
 * xinclude and tagging.
 *
 * Support two methods:
 *	1. load - loading file
 *	2. include - loading file with xinclude processing.
 *
 * Both methods support tagging based on file modification time.
 */
class FileBlock : public ThreadedTaggedBlock {
public:
    FileBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~FileBlock();

protected:
    virtual void postParse();
    virtual XmlDocHelper call(Context *ctx, boost::any &a) throw (std::exception);

    /**
     * Loading file with optional xinclude processing.
     */
    XmlDocHelper loadFile(const std::string& file_name, Context *ctx);

    /**
     * Create full filename based on relative name in first arg.
     */

    XmlDocHelper invokeFile(const std::string& file_name, Context *ctx);

private:
    Method method_;
    bool processXInclude_;
};


}

#endif
