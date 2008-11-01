#ifndef _XSCRIPT_VALIDATOR_BASE_H
#define _XSCRIPT_VALIDATOR_BASE_H

#include <libxml/tree.h>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

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

        /// Check validator. Factory method around checkImpl and will set guard
        /// in case of errors.
        void check(const Context *ctx, const Param &param) const;

        /// Get guard name.
        const std::string& guardName() const {
            return guard_name_;
        }
    protected:
        /// Check. Throw ValidatorException if validator failed.
        virtual void checkImpl(const Context *ctx, const Param &value) const = 0;

        /// id of param
        std::string param_id_;

        /// State param to set to in case of errors.
        std::string guard_name_;
    };


    /**
     * Virtual constructor for Validators. 
     * boost::function<ValidatorBase* (xmlNodePtr)>.
     */
    typedef boost::function<Validator* (xmlNodePtr node)> ValidatorConstructor;

    class ValidatorRegisterer : private boost::noncopyable {
    public:
        ValidatorRegisterer(const char *name, const ValidatorConstructor &cons);
    };

}

#endif
