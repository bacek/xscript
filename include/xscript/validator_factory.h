#ifndef _XSCRIPT_VALIDATOR_FACTORY_H
#define _XSCRIPT_VALIDATOR_FACTORY_H

#include <map>
#include <utility>
#include <xscript/component.h>
#include <xscript/validator.h>

namespace xscript
{
    /**
     * Factory for creating Validators for Params.
     *
     * Mail validation things:
     * 1. We validate each Param independently.
     * 2. In case of validation failure we emit &lt;xscript_invoke_failed
     * validate="failed" reason="Invalid something"/&gt; node. # FIXME We don't
     * actually ATM.
     * 3. Specification of validator for param looks like
          &lt;param ... validator="type" ... validate-error-guard="guard_name"&gt;. 
          validate-error-guard - optional State guard which set in case of validation failure.
     * 4. All validators registered in this factory.
     * 5. We can add Validators in run-time for additional checks.
     */

    class ValidatorFactory : public Component<ValidatorFactory> {
    public:
        virtual ~ValidatorFactory();
        friend class Component<ValidatorFactory>;

        /**
         * Create (optional) validator for <param>.
         * \param node <param> xmlNode
         * \return Validator for param. NULL if validator wasn't requested.
         */
        std::auto_ptr<Validator> createValidator(xmlNodePtr node) const;

        /**
         * Registring validator creator.
         * \param type Type of validator
         * \param ctor Validator constructor.
         * \return void. Throw std::runtime_error in case of type duplicate.
         */
        void registerConstructor(
            const std::string           &type, 
            const ValidatorConstructor  &ctor);

    private:
        ValidatorFactory();

        /// Map for storing Validator creators.
        typedef std::map<std::string, ValidatorConstructor> ValidatorMap;
        ValidatorMap validator_creators_;
    };
}

#endif
