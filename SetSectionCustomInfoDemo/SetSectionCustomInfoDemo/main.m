//
//  main.m
//  SetSectionCustomInfoDemo
//
//  Created by kyson on 2021/4/10.
//

#import <Foundation/Foundation.h>
#import <dlfcn.h>
#include <mach-o/loader.h>
#include <mach-o/getsect.h>

#ifndef __LP64__
#define mach_header mach_header
#else
#define mach_header mach_header_64
#endif

const struct mach_header *machHeader = NULL;
static NSString *configuration = @"";

//设置"__DATA,__customSection"的数据为kyson.cn
char *kString __attribute__((section("__DATA,__customSection"))) = (char *)"kyson.cn";

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        //设置machheader信息
        if (machHeader == NULL) {
            Dl_info info;
            dladdr((__bridge const void *)(configuration), &info);
            machHeader = (struct mach_header_64*)info.dli_fbase;
        }

        unsigned long byteCount = 0;
        uintptr_t* data = (uintptr_t *) getsectiondata(machHeader, "__DATA", "__customSection", &byteCount);
        NSUInteger counter = byteCount/sizeof(void*);
        for(NSUInteger idx = 0; idx < counter; ++idx) {
            char *string = (char*)data[idx];
            NSString *str = [NSString stringWithUTF8String:string];
            NSLog(@"%@",str);
        }
    }
    return 0;
}
