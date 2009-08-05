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
 * replace method
 */

string Pcre::replace(const string& piece, const string& with) {
  /*
   * Pcre::replace version by "Marcus Kramer" <marcus.kramer@scherrer.de>
   */
  string Replaced(piece);

  bool bReplaced = false;
  int  iReplaced = -1;

  __pcredebug << "replace: " << piece << " with: " << with << endl;

  /*
   * certainly we need an anchor, we want to check if the whole arg is in brackets
   * //Pcre braces("^[^\\\\]\\(.*[^\\\\]\\)$"); // perlish: [^\\]\(.*[^\\]\)
   *
   * There's no reason, not to add brackets in general.
   * It's more comfortable, cause we wants to start with $1 at all, 
   * also if we set the whole arg in brackets!
   */
  
  /* recreate the p_pcre* objects to avoid memory leaks */
  pcre_free(p_pcre);
  pcre_free(p_pcre_extra);
  
  pcre       *_p = NULL;
  pcre_extra *_e = NULL;;
        
  p_pcre = _p;
  p_pcre_extra = _e;
  
  if (! _have_paren ) {
    string::size_type p_open, p_close;
    p_open  = _expression.find_first_of("(");
    p_close = _expression.find_first_of(")");
    if ( p_open == string::npos && p_close == string::npos ) {
      /*
       * Well, _expression completely lacks of parens, which are
       * required for search/replace operation. So add parens automatically.
       * Note, that we add 2 pairs of parens so that we finally have $0 matchin
       * the whole expression and $1 matching the inner side (which is in this
       * case the very same string.
       * We do this for perl compatibility. If the expression already contains
       * parens, the whole match will produce $0 for us, so in this case we
       * have no problem
       */
      _expression = "((" + _expression + "))";
    }
    else {
      /*
       * Add parens to the very beginning and end of the expression
       * so that we have $0. I don't care if the user already supplied
       * double-parentesed experssion (e.g. "((1))"), because PCRE seems
       * to eat redundant parenteses, e.g. "((((1))))" returns the same
       * result as "((1))".
       */
      _expression = "(" + _expression;
      _expression=_expression + ")"; 
    }

    _have_paren = true;
  }

  __pcredebug << "_expression: " << _expression << endl;

  Compile(_flags);
        
  if(search(piece)) {
    /* we found at least one match */
    
    // sure we must resolve $1 for ever piece we found especially for "g"
    // so let's just create that var, we resolve it when we needed!
    string use_with;


    if(!global_t) {
      // here we can resolve vars if option g is not set
      use_with = _replace_vars(with);

      if(matched() && matches() >= 1) {
	__pcredebug << "matches: " << matches() << endl;
	int len = get_match_end() - get_match_start() + 1;
	Replaced.replace(get_match_start(0), len, use_with);
	bReplaced  = true;
	iReplaced = 0;
      }
    }
    else {
      /* global replace */

      // in global replace we just need to remember our position
      // so let's initialize it first
      int match_pos = 0;
      while( search( Replaced, match_pos ) ) {
	int len = 0;
                                
	// here we need to resolve the vars certainly for every hit.
	// could be different content sometimes!
	use_with = _replace_vars(with);
                                
	len = get_match_end() - get_match_start() + 1;
	Replaced.replace(get_match_start(0), len, use_with);
                                
	//# Next run should begin after the last char of the stuff we put in the text
	match_pos = ( use_with.length() - len ) + get_match_end() + 1;

	bReplaced  = true;
	++iReplaced;
      }
    }
  }
  
  did_match   = bReplaced;
  num_matches = iReplaced;

  return Replaced;
}





string Pcre::_replace_vars(const string& piece) {
  /*
   * Pcre::_replace_vars version by "Marcus Kramer" <marcus.kramer@scherrer.de>
   */
  string with  = piece;
  Pcre dollar("\\$([0-9]+)");

  while ( dollar.search( with ) ) {
    // let's do some conversion first
    __pcredebug << "Pcre::dollar matched: " << piece << ". Match(0): " << dollar.get_match(0) << endl;
    int iBracketIndex = atoi( dollar.get_match(0).data() );
    string sBracketContent = get_match(iBracketIndex);
    
    // now we can splitt the stuff
    string sSubSplit = "\\$" + dollar.get_match(0);
    Pcre subsplit(sSubSplit);
                
    // normally 2 (or more) parts, the one in front of and the other one after "$1"
    vector<string> splitted = subsplit.split(with); 
    string Replaced;
                
    for(size_t pos=0; pos < splitted.size(); pos++) {
      if( pos == ( splitted.size() - 1 ) ) 
	Replaced += splitted[pos];
      else 
	Replaced += splitted[pos] + sBracketContent;
    }
    with = Replaced; // well, one part is done
  }
  return with;
}
