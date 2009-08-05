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
 * split method and family
 */

vector<string> Pcre::_split(const string& piece, int limit, int start_offset, int end_offset) {
  vector<string> Splitted;
  /* _expression will be used as delimiter */
  if(_expression.length() == 1) {
    /* use the plain c++ way, ignore the pre-compiled p_pcre */
    string buffer, _delimiter, _piece;
    char z;
    if(case_t) {
      z = toupper(_expression[0]);
      for(size_t pos=0; pos < piece.length(); pos++) {
	_piece += (char)toupper(piece[pos]);
      }
    }
    else {
      z = _expression[0];
      _piece = piece;
    }
    for(size_t pos=0; pos<piece.length(); pos++) {
      if(_piece[pos] == z) {
	Splitted.push_back(buffer);
	buffer = "";
      }
      else {
	buffer += piece[pos];
      }
    }
    if(buffer != "") {
      Splitted.push_back(buffer);
    }
  }
  else {
    /* use the regex way */
    if(_expression[0] != '(' && _expression[ _expression.length() - 1 ] != ')' ) {
      /* oh, oh - the pre-compiled expression does not contain brackets */
      pcre_free(p_pcre);
      pcre_free(p_pcre_extra);
      
      pcre       *_p = NULL;
      pcre_extra *_e = NULL;;

      p_pcre = _p;
      p_pcre_extra = _e;

      _expression = "(" + _expression + ")";
      Compile(_flags);
    }
    int num_pieces=0, pos=0, piece_end = 0, piece_start = 0;
    for(;;) {
      if(search(piece, pos) == true) {
	if(matches() > 0) {
	  piece_end   = get_match_start(0) - 1;
	  piece_start = pos;
	  pos = piece_end + 1 + get_match_length(0);
	  string junk(piece, piece_start, (piece_end - piece_start)+1);
	  num_pieces++;
	  if( (limit != 0 && num_pieces < limit) || limit == 0) {
	    if( (start_offset != 0 && num_pieces >= start_offset) || start_offset == 0) {
	      if( (end_offset != 0 && num_pieces <= end_offset) || end_offset == 0) {
		/* we are within the allowed range, so just add the grab */
		Splitted.push_back(junk);
	      }
	    }
	  }
	}
      }
      else {
	/* the rest of the string, there are no more delimiters */
	string junk(piece, pos, (piece.length() - pos));
	num_pieces++;
	if( (limit != 0 && num_pieces < limit) || limit == 0) {
	  if( (start_offset != 0 && num_pieces >= start_offset) || start_offset == 0) {
	    if( (end_offset != 0 && num_pieces <= end_offset) || end_offset == 0) {
	      /* we are within the allowed range, so just add the grab */
	      Splitted.push_back(junk);
	    }
	  }
	}
	break;
      }
    } // for()
  } // if(_expression.length()
  return Splitted;
}

vector<string> Pcre::split(const string& piece) {
  return _split(piece, 0, 0, 0);
}

vector<string> Pcre::split(const string& piece, int limit) {
  return _split(piece, limit, 0, 0);
}

vector<string> Pcre::split(const string& piece, int limit, int start_offset) {
  return _split(piece, limit, start_offset, 0);
}

vector<string> Pcre::split(const string& piece, int limit, int start_offset, int end_offset) {
  return _split(piece, limit, start_offset, end_offset);
}

vector<string> Pcre::split(const string& piece, vector<int> positions) {
  vector<string> PreSplitted = _split(piece, 0, 0, 0);
  vector<string> Splitted;
  for(vector<int>::iterator vecIt=positions.begin(); vecIt != positions.end(); ++vecIt) {
    Splitted.push_back(PreSplitted[*vecIt]);
  }
  return Splitted;
}
