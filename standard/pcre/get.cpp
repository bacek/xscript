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

using namespace pcrepp;

/*
 * get_*() methods which return (sub)informations such as matches
 * or strings
 */


std::vector<std::string>*
Pcre::get_sub_strings() const {
    return resultset ? resultset : NULL;
}

std::string
Pcre::get_match(int pos) const {
    if (pos >= 0 && pos < num_matches) {
        std::vector<std::string>::iterator p = resultset->begin() + pos;
        return *p;
    }
    throw exception("Pcre::get_match(int): out of range");
}

std::string
Pcre::get_match(int pos) {
	return static_cast<const Pcre*>(this)->get_match(pos);
}

int
Pcre::get_match_start() const {
    if (sub_vec) {
        return sub_vec[0];
    }
    return -1;
}

int
Pcre::get_match_end() const {
    if (sub_vec) {
        return sub_vec[1] - 1;
    }
    return -1;
}

int
Pcre::get_match_start(int pos) const {
    if (pos >= 0 && pos <= num_matches) {
    /*
     * sub_vec[0] and [1] is the start/end of the entire string.
     */
        return sub_vec[(++pos) * 2];
    }
    throw exception("Pcre::get_match_start(int): out of range");  
}

int
Pcre::get_match_end(int pos) const {
    if(pos >= 0 && pos <= num_matches) {
    /*
     * the end offset of a subpattern points to
     * the first offset of the next substring,
     * therefore -1
     */
        return sub_vec[((++pos) * 2) + 1] - 1;
    }
    throw exception("Pcre::get_match_end(int): out of range");
}

size_t
Pcre::get_match_length(int pos) const {
    if(pos >= 0 && pos < num_matches) {
        std::vector<std::string>::iterator p = resultset->begin() + pos;
        return p->length();
    }
    throw exception("Pcre::get_match_length(int): out of range");
}
