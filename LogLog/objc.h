//
//  objc.h
//  ObjcLogSystem
//
//  Created by kyson on 2021/3/29.
//

#ifndef _OBJC_OBJC_H_
#define _OBJC_OBJC_H_


#include <stdbool.h>


#if OBJC_BOOL_IS_BOOL
    typedef bool BOOL;
#else
#   define OBJC_BOOL_IS_CHAR 1
    typedef signed char BOOL;
    // BOOL is explicitly signed so @encode(BOOL) == "c" rather than "C"
    // even if -funsigned-char is used.
#endif

#ifndef nil
# if __has_feature(cxx_nullptr)
#   define nil nullptr
# else
#   define nil __DARWIN_NULL
# endif
#endif



#endif /* _OBJC_OBJC_H_ */
