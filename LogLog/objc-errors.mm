//
//  objc-errors.m
//  ObjcLogSystem
//
//  Created by kyson on 2021/3/29.
//

#import <Foundation/Foundation.h>
#include "objc-private.h"


#include <_simple.h>


// Returns true if logs should be sent to stderr as well as syslog.
// Copied from CFUtilities.c
static bool also_do_stderr(void)
{
    struct stat st;
    int ret = fstat(STDERR_FILENO, &st);
    if (ret < 0) return false;
    mode_t m = st.st_mode & S_IFMT;
    if (m == S_IFREG  ||  m == S_IFSOCK  ||  m == S_IFIFO  ||  m == S_IFCHR) {
        return true;
    }
    return false;
}

// Print "message" to the console.
static void _objc_syslog(const char *message)
{
    _simple_asl_log(ASL_LEVEL_ERR, nil, message);

    if (also_do_stderr()) {
        write(STDERR_FILENO, message, strlen(message));
    }
}



/*
 * this routine handles soft runtime errors...like not being able
 * add a category to a class (because it wasn't linked in).
 */
void _objc_inform(const char *fmt, ...)
{
    va_list ap;
    char *buf1;
    char *buf2;

    va_start (ap,fmt);
    vasprintf(&buf1, fmt, ap);
    va_end (ap);

    asprintf(&buf2, "objc[%d]: %s\n", getpid(), buf1);
    _objc_syslog(buf2);

    free(buf2);
    free(buf1);
}


