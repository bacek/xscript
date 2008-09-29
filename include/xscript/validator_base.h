#ifndef _XSCRIPT_VALIDATOR_BASE_H
#define _XSCRIPT_VALIDATOR_BASE_H

#include <libxml/tree.h>
#include <boost/function.hpp>

namespace xscript
{
    class Context;

    /**
     * Base class for validating parameters.
     */
    class ValidatorBase {
    public:
        ValidatorBase(xmlNodePtr node);
        virtual ~ValidatorBase();

        /// Check validator. Factory method around isFailed and will set guard
        /// and throw ValidatorException if case of errors.
        void check(const Context *ctx, const std::string &value) const;

        /// Get guard name.
        const std::string& guardName() const {
            return guard_name_;
        }
    protected:
        /// Check. Return true if validator failed.
        virtual bool isFailed(const std::string &value) const = 0;

        /// State param to set to in case of errors.
        std::string guard_name_;
    };


    /**
     * Virtual constructor for Validators. 
     * boost::function<ValidatorBase* (xmlNodePtr)>.
     */
    typedef boost::function<ValidatorBase* (xmlNodePtr node)> ValidatorConstructor;
}

#endif
