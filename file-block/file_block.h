#ifndef _XSCRIPT_FILE_BLOCK_H_
#define _XSCRIPT_FILE_BLOCK_H_

#include <boost/function.hpp>
#include <xscript/extension.h>
#include <xscript/tagged_block.h>
#include <xscript/threaded_block.h>

namespace xscript {
class FileBlock;

typedef XmlDocHelper (FileBlock::*Method)(const std::string&,
        boost::shared_ptr<Context>, boost::shared_ptr<InvokeContext>) const;

/**
 * x:file block. Loading local files with <s>jackpot and hookers</s> with
 * xinclude and tagging.
 *
 * Support two methods:
 *    1. load - loading file
 *    2. include - loading file with xinclude processing.
 *
 * Both methods support tagging based on file modification time.
 */
class FileBlock : public ThreadedBlock, public TaggedBlock {
public:
    FileBlock(const Extension *ext, Xml *owner, xmlNodePtr node);
    virtual ~FileBlock();

    std::string createTagKey(const Context *ctx) const;
    virtual bool allowDistributed() const;
private:
    virtual void postParse();
    virtual void call(boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const throw (std::exception);

    XmlDocHelper loadFile(const std::string &file_name,
            boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const;

    XmlDocHelper loadText(const std::string &file_name,
            boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const;

    XmlDocHelper loadBinary(const std::string &file_name,
            boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const;
    
    XmlDocHelper invokeFile(const std::string &file_name,
            boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const;
    
    XmlDocHelper invokeMethod(const std::string &file_name,
            boost::shared_ptr<Context> ctx, boost::shared_ptr<InvokeContext> invoke_ctx) const;
    
    XmlDocHelper testFileDoc(bool result, const std::string &file) const;
    
    std::string fileName(const Context *ctx) const;
    
    virtual void property(const char *name, const char *value);
    
    bool isTest() const;
    bool isInvoke() const;

private:
    Method method_;
    bool processXInclude_;
    bool ignore_not_existed_;
    
    static const std::string FILENAME_PARAMNAME;
    static const std::string INVOKE_SCRIPT_PARAMNAME;
};


}

#endif
