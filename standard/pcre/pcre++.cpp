/*
 *
 *  This file  is part of the PCRE++ Class Library.
 *
 *  By  accessing  this software,  PCRE++, you  are  duly informed
 *  of and agree to be  bound  by the  conditions  described below
 *  in this notice:
 *
 *  This software product,  PCRE++,  is developed by Thomas Linden
 *  and copyrighted (C) 2002-2003 by Thomas Linden,with all rights 
 *  reserved.
 *
 *  There  is no charge for PCRE++ software.  You can redistribute
 *  it and/or modify it under the terms of the GNU  Lesser General
 *  Public License, which is incorporated by reference herein.
 *
 *  PCRE++ is distributed WITHOUT ANY WARRANTY, IMPLIED OR EXPRESS,
 *  OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE or that
 *  the use of it will not infringe on any third party's intellec-
 *  tual property rights.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with PCRE++.  Copies can also be obtained from:
 *
 *    http://www.gnu.org/licenses/lgpl.txt
 *
 *  or by writing to:
 *
 *  Free Software Foundation, Inc.
 *  59 Temple Place, Suite 330
 *  Boston, MA 02111-1307
 *  USA
 *
 *  Or contact:
 *
 *   "Thomas Linden" <tom@daemon.de>
 *
 *
 */


#include "pcre++.h"

using namespace xscript::pcrepp;

/*
 * CONSTRUCTORS and stuff
 */
Pcre::Pcre(const std::string &expression) {
    _have_paren = false;
    _expression = expression;
    _flags = PCRE_UTF8; // Yandex fix
    case_t = global_t = false;
    zero();
    Compile(0);
}

Pcre::Pcre(const std::string &expression, const std::string &flags) {
    _have_paren = false;
    _expression = expression;
    unsigned int FLAG = PCRE_UTF8; // Yandex fix

    for(unsigned int flag = 0; flag < flags.length(); ++flag) {
        switch(flags[flag]) {
            case 'i': FLAG |= PCRE_CASELESS; case_t = true; break;
            case 'm': FLAG |= PCRE_MULTILINE; break;
            case 's': FLAG |= PCRE_DOTALL; break;
            case 'x': FLAG |= PCRE_EXTENDED; break;
            case 'g': global_t = true; break;
        }
    }

    _flags = FLAG;
    zero();
    Compile(FLAG);
}

Pcre::Pcre(const std::string &expression, unsigned int flags) {
    _have_paren    = false;
    _expression = expression;
    _flags = flags;

    if ((_flags & PCRE_CASELESS) != 0) {
        case_t = true;
    }

    if ((_flags & PCRE_GLOBAL) != 0) {
        global_t = true;
        _flags = _flags - PCRE_GLOBAL; /* remove pcre++ flag before feeding _flags to pcre */
    }

    zero();
    Compile(_flags);
}

Pcre::Pcre(const Pcre &P) {
    _have_paren = P._have_paren;
    _expression = P._expression;
    _flags = P._flags;
    case_t = P.case_t;
    global_t = P.global_t;
    zero();
    Compile(_flags);
}

Pcre::Pcre() {
    zero();
}

/*
 * Destructor
 */
Pcre::~Pcre() {
  /* avoid deleting of uninitialized pointers */
    if (p_pcre != NULL) {
        pcre_free(p_pcre);
    }
    if (p_pcre_extra != NULL) {
        pcre_free(p_pcre_extra);
    }
    if(sub_vec != NULL) {
        delete[] sub_vec;
    }
    if(resultset != NULL) {
        delete resultset;
    }
}

/*
 * operator= definitions
 */
const Pcre&
Pcre::operator = (const std::string &expression) {
  /* reset the object and re-intialize it */
    reset();
    _expression = expression;
    _flags = 0;
    case_t = global_t = false;
    Compile(0);
    return *this;
}

const Pcre&
Pcre::operator = (const Pcre &P) {
    reset();
    _expression = P._expression;
    _flags = P._flags;
    case_t = P.case_t;
    global_t = P.global_t;
    zero();
    Compile(_flags);
    return *this;
}

/*
 * mem resetting methods
 */
void
Pcre::zero() {
  /* what happens if p_pcre is already allocated? hm ... */
    p_pcre_extra = NULL;
    p_pcre = NULL;
    sub_vec = NULL;
    resultset = NULL;
    err_str = NULL;
    num_matches = -1;
    tables = NULL;
}

void
Pcre::reset() {
    did_match = false;
    num_matches = -1;
}

/*
 * accessing the underlying implementation
 */
pcre*
Pcre::get_pcre() {
    return p_pcre;
}

pcre_extra*
Pcre::get_pcre_extra() {
    return p_pcre_extra;
}

/*
 * support stuff
 */
void
Pcre::study() {
    p_pcre_extra = pcre_study(p_pcre, 0, (const char **)(&err_str));
    if(err_str != NULL) {
        throw exception("pcre_study(..) failed: " + std::string(err_str));
    }
}

/*
 * setting current locale
 */
bool
Pcre::setlocale(const char* locale) {
    if (std::setlocale(LC_CTYPE, locale) == NULL) {
        return false;
    }
    tables = pcre_maketables();
    return true;
}
