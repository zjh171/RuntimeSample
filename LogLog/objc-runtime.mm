//
//  objc-runtime.m
//  ObjcLogSystem
//
//  Created by kyson on 2021/3/29.
//
//#   include <unistd.h>
//#   include <string.h>
//#   include <crt_externs.h>
//#import <Foundation/Foundation.h>

#include "objc-private.h"


// Settings from environment variables
#define OPTION(var, env, help) bool var = false;
#include "objc-env.h"
#undef OPTION


struct option_t {
    bool* var;
    const char *env;
    const char *help;
    size_t envlen;
};


const option_t Settings[] = {
#define OPTION(var, env, help) option_t{&var, #env, help, strlen(#env)},
#include "objc-env.h"
#undef OPTION
};


namespace objc {
    int PageCountWarning = 50;  // Default value if the environment variable is not set
}


/***********************************************************************
* SetPageCountWarning
* Convert environment variable value to integer value.
* If the value is valid, set the global PageCountWarning value.
**********************************************************************/
void SetPageCountWarning(const char* envvar) {
    if (envvar) {
        long result = strtol(envvar, NULL, 10);
        if (result <= INT_MAX && result >= -1) {
            int32_t var = (int32_t)result;
            if (var != 0) {  // 0 is not a valid value for the env var
                objc::PageCountWarning = var;
            }
        }
    }
}

/***********************************************************************
* environ_init
* Read environment variables that affect the runtime.
* Also print environment variable help, if requested.
**********************************************************************/
void environ_init(void)
{
    if (issetugid()) {
        // All environment variables are silently ignored when setuid or setgid
        // This includes OBJC_HELP and OBJC_PRINT_OPTIONS themselves.
        return;
    }

    // Turn off autorelease LRU coalescing by default for apps linked against
    // older SDKs. LRU coalescing can reorder releases and certain older apps
    // are accidentally relying on the ordering.
    // rdar://problem/63886091
//    if (!dyld_program_sdk_at_least(dyld_fall_2020_os_versions))
//        DisableAutoreleaseCoalescingLRU = true;

    bool PrintHelp = false;
    bool PrintOptions = false;
    bool maybeMallocDebugging = false;

    // Scan environ[] directly instead of calling getenv() a lot.
    // This optimizes the case where none are set.
    for (char **p = *_NSGetEnviron(); *p != nil; p++) {
        if (0 == strncmp(*p, "Malloc", 6)  ||  0 == strncmp(*p, "DYLD", 4)  ||
            0 == strncmp(*p, "NSZombiesEnabled", 16))
        {
            maybeMallocDebugging = true;
        }

        if (0 != strncmp(*p, "OBJC_", 5)) continue;
        
        if (0 == strncmp(*p, "OBJC_HELP=", 10)) {
            PrintHelp = true;
            continue;
        }
        if (0 == strncmp(*p, "OBJC_PRINT_OPTIONS=", 19)) {
            PrintOptions = true;
            continue;
        }
        
        if (0 == strncmp(*p, "OBJC_DEBUG_POOL_DEPTH=", 22)) {
            SetPageCountWarning(*p + 22);
            continue;
        }

        const char *value = strchr(*p, '=');
        if (!*value) continue;
        value++;
        
        for (size_t i = 0; i < sizeof(Settings)/sizeof(Settings[0]); i++) {
            const option_t *opt = &Settings[i];
            if ((size_t)(value - *p) == 1+opt->envlen  &&
                0 == strncmp(*p, opt->env, opt->envlen))
            {
                *opt->var = (0 == strcmp(value, "YES"));
                break;
            }
        }
    }

    // Special case: enable some autorelease pool debugging
    // when some malloc debugging is enabled
    // and OBJC_DEBUG_POOL_ALLOCATION is not set to something other than NO.
    if (maybeMallocDebugging) {
        const char *insert = getenv("DYLD_INSERT_LIBRARIES");
        const char *zombie = getenv("NSZombiesEnabled");
        const char *pooldebug = getenv("OBJC_DEBUG_POOL_ALLOCATION");
        if ((getenv("MallocStackLogging")
             || getenv("MallocStackLoggingNoCompact")
             || (zombie && (*zombie == 'Y' || *zombie == 'y'))
             || (insert && strstr(insert, "libgmalloc")))
            &&
            (!pooldebug || 0 == strcmp(pooldebug, "YES")))
        {
            DebugPoolAllocation = true;
        }
    }

//    if (!os_feature_enabled_simple(objc4, preoptimizedCaches, true)) {
//        DisablePreoptCaches = true;
//    }

    // Print OBJC_HELP and OBJC_PRINT_OPTIONS output.
    if (PrintHelp  ||  PrintOptions) {
        if (PrintHelp) {
            _objc_inform("Objective-C runtime debugging. Set variable=YES to enable.");
            _objc_inform("OBJC_HELP: describe available environment variables");
            if (PrintOptions) {
                _objc_inform("OBJC_HELP is set");
            }
            _objc_inform("OBJC_PRINT_OPTIONS: list which options are set");
        }
        if (PrintOptions) {
            _objc_inform("OBJC_PRINT_OPTIONS is set");
        }

        for (size_t i = 0; i < sizeof(Settings)/sizeof(Settings[0]); i++) {
            const option_t *opt = &Settings[i];
            if (PrintHelp) _objc_inform("%s: %s", opt->env, opt->help);
            if (PrintOptions && *opt->var) _objc_inform("%s is set", opt->env);
        }
    }
}
