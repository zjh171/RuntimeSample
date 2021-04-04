//
//  main.m
//  LogDemo
//
//  Created by kyson on 2021/4/3.
//

#import <Foundation/Foundation.h>
#import "objc-private.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"Hello, World!");
        environ_init();
        
        _objc_inform("123456555555555");
    }
    return 0;
}
