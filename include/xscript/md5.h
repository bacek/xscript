/* MD5.H - header file for MD5C.C
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */
#ifndef _XSCRIPT_MD5_H_
#define _XSCRIPT_MD5_H_

#include "settings.h"
#include <boost/cstdint.hpp>
#include <string>

namespace xscript
{
/* MD5 context. */
typedef struct {
	boost::uint32_t state[4];	/* state (ABCD) */
	boost::uint32_t count[2];	/* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];	/* input buffer */
} MD5_CTX;

void MD5_Init(MD5_CTX *);
void MD5_Update(MD5_CTX *, const unsigned char *, unsigned int);
void MD5_Final(unsigned char[16], MD5_CTX *);
std::string MD5_Hex(const char *key);

}

#endif //namespace xscript