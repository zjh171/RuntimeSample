//
//  main.m
//  debugobjc
//
//  Created by cooci on 2021/1/5.


#import <Foundation/Foundation.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"Hello, World!");
        NSObject *objc = [NSObject alloc];
        [objc isKindOfClass:NSObject.class];
    }
    return 0;
}
