#ifndef _XSCRIPT_VALIDATOR_FACTORY_H
#define _XSCRIPT_VALIDATOR_FACTORY_H

#include <map>
#include <utility>
#include <xscript/component.h>
#include <xscript/validator_base.h>

namespace xscript
{
    /**
     * 
     * Sorry, guys. I'll translate this later.
     *
     * 1. Валидируем по параметру.
     * 2. Если валидация профейлилась выдаём <xscript_invoke_failed validate="failed" reason="Invalid something"/>.
     * 3. Валидация параметров указывается так: <param ... validate="type" ...
     * validate-error-guard="guard_name">. validate-error-guard - опциональный.
     * 4. Делаем комбо ValidatorFactory/ValidatorBase/ValidateException.
     * 5. В фабрике регистрируем конструкторы по имени (использующихся в validate="type")
     * 6. При пасинге параметра, если встретили аттрибут validate, идём в фабрику и скармливаем ей текущую ноду. Тогда разные валидаторы могут подтянуть дополнительные аттрибуты.
     * 7. При вызове валидатор может бростить ValidateException. Если мы его ловим, то выставляем validate-error-guard в true, ex.what() записываем в xscript_invoke_failed/@reason.
     */

    class ValidatorFactory : public Component<ValidatorFactory> {
    public:
        friend class ComponentRegisterer<ValidatorFactory>;

        /**
         * Create (optional) validator for <param>.
         * \param node <param> xmlNode
         * \return Validator for param. NULL if validator wasn't requested.
         */
        std::auto_ptr<ValidatorBase> createValidator(xmlNodePtr node);

    private:
        ValidatorFactory();
        virtual ~ValidatorFactory();

        /// Map for storing Validator creators.
        std::map<std::string, ValidatorConstructor*> validator_creators_;
    };
}

#endif