#ifndef _XSCRIPT_EXPECT_H_
#define _XSCRIPT_EXPECT_H_

#ifdef HAVE_BUILTIN_EXPECT

#define XSCRIPT_LIKELY(expr) __builtin_expect(expr, 1)
#define XSCRIPT_UNLIKELY(expr) __builtin_expect(expr, 0)

#else

#define XSCRIPT_LIKELY(expr) expr
#define XSCRIPT_UNLIKELY(expr) expr

#endif

#endif // _XSCRIPT_EXPECT_H_
