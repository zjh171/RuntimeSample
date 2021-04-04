//
//  main.m
//  debugobjc
//
//  Created by cooci on 2021/1/5.


#import <Foundation/Foundation.h>


@interface DemoClass : NSObject

@end

@implementation DemoClass



@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        DemoClass *objc = [[DemoClass alloc] init];
        [objc isKindOfClass:NSObject.class];
    }
    return 0;
}
