//
//  objc-private.h
//  ObjcLogSystem
//
//  Created by kyson on 2021/3/27.
//

#ifndef _OBJC_PRIVATE_H_
#define _OBJC_PRIVATE_H_

#ifdef _OBJC_OBJC_H_
//#error include objc-private.h before other headers
#endif

#include <stdio.h>

// Public headers
#include "objc.h"
#include "objc-os.h"


#define OPTION(var, env, help) extern bool var;
#include "objc-env.h"
#undef OPTION




/**
 errors
 
 */

extern void _objc_inform(const char *fmt, ...) __attribute__((cold, format(printf, 1, 2)));


extern void environ_init(void);

#endif /* objc_private_h */
