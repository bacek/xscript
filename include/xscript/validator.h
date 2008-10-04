#ifndef _XSCRIPT_VALIDATOR_BASE_H
#define _XSCRIPT_VALIDATOR_BASE_H

#include <libxml/tree.h>
#include <boost/function.hpp>

namespace xscript
{
    class Context;
    class Param;

    /**
     * Base class for validating parameters.
     */
    class Validator {
    public:
        Validator(xmlNodePtr node);
        virtual ~Validator();

        /// Check validator. Factory method around isFailed and will set guard
        /// and throw ValidatorException if case of errors.
        void check(const Context *ctx, const Param &param) const;

        /// Get guard name.
        const std::string& guardName() const {
            return guard_name_;
        }
    protected:
        /// Check. Return true if validator passed.
        virtual bool isPassed(const Param &value) const = 0;

        /// State param to set to in case of errors.
        std::string guard_name_;
    };


    /**
     * Virtual constructor for Validators. 
     * boost::function<ValidatorBase* (xmlNodePtr)>.
     */
    typedef boost::function<Validator* (xmlNodePtr node)> ValidatorConstructor;
}

#endif
