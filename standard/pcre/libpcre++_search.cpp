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

using namespace std;
using namespace pcrepp;

/*
 * the search interface to pcre
 */


/*
 * compile the expression
 */
void Pcre::Compile(int flags) {
  p_pcre       = pcre_compile((char *)_expression.c_str(), flags,
			      (const char **)(&err_str), &erroffset, tables);

  if(p_pcre == NULL) {
    /* umh, that's odd, the parser should not fail at all */
    string Error = err_str;
    throw exception("pcre_compile(..) failed: " + Error + " at: " + _expression.substr(erroffset));
  }

  /* calculate the number of substrings we are willing to catch */
  int where;
  int info = pcre_fullinfo( p_pcre, p_pcre_extra, PCRE_INFO_CAPTURECOUNT, &where);
  if(info == 0) {
    sub_len = (where +2) * 3; /* see "man pcre" for the exact formula */
  }
  else {
    throw exception(info);
  }
  reset();
}




/*
 * API methods
 */
bool Pcre::search(const string& stuff, int OffSet){
  return dosearch(stuff, OffSet);
}

bool Pcre::search(const string& stuff){
  return dosearch(stuff, 0);
}

bool Pcre::dosearch(const string& stuff, int OffSet){
  reset();
  if (sub_vec != NULL)
    delete[] sub_vec;

  sub_vec = new int[sub_len];
  int num = pcre_exec(p_pcre, p_pcre_extra, (char *)stuff.c_str(),
                        (int)stuff.length(), OffSet, 0, (int *)sub_vec, sub_len);

  __pcredebug << "Pcre::dosearch(): pcre_exec() returned: " << num << endl;

  if(num < 0) {
    /* no match at all */
    __pcredebug << " - no match" << endl;
    return false;
  }
  else if(num == 0) {
    /* vector too small, there were too many substrings in stuff */
    __pcredebug << " - too many substrings" << endl;
    return false;
  }
  else if(num == 1) {
    /* we had a match, but without substrings */
    __pcredebug << " - match without substrings" << endl;
    did_match = true;
    num_matches = 0;
    return true;
  }
  else if(num > 1) {
    /* we had matching substrings */
    if (resultset != NULL)
      delete resultset;
    resultset = new vector<string>;
    const char **stringlist;
    did_match = true;
    num_matches = num - 1;

    __pcredebug << " - match with " << num_matches << " substrings" << endl;

    int res = pcre_get_substring_list((char *)stuff.c_str(), sub_vec, num, &stringlist);
    if(res == 0) {
      __pcredebug << "Pcre::dosearch(): matched substrings: " << endl;
      for(int i=1; i<num; i++) {
	__pcredebug << " " << string(stringlist[i]) << endl;
	resultset->push_back(stringlist[i]);
      }
      pcre_free_substring_list(stringlist);
    }
    else {
      throw exception(res);
    }
    return true;
  }
  else {
    /* some other uncommon error occured */
    __pcredebug << " - uncommon error" << endl;
    return false;
  }
}
