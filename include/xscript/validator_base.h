#ifndef _XSCRIPT_VALIDATOR_BASE_H
#define _XSCRIPT_VALIDATOR_BASE_H

#include <libxml/tree.h>
#include <boost/function.hpp>


namespace xscript
{
    /**
     * Base class for validating parameters.
     */
    class ValidatorBase {
    public:
        virtual ~ValidatorBase();

    protected:
    };


    /**
     * Virtual constructor for Validators. 
     * boost::function<ValidatorBase* (xmlNodePtr)>.
     */
    typedef boost::function<ValidatorBase* (xmlNodePtr node)> ValidatorConstructor;
}

#endif