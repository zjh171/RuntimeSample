//
//  TestObject.m
//  debug-objc
//
//  Created by kyson on 2019/1/30.
//

#import "TestObject.h"



@implementation TestObject

//+(void) load {
//    NSLog(@"hello");
//    A();
//}

+(instancetype)allocWithZone:(struct _NSZone *)zone {
    TestObject *obj = [[TestObject alloc] init];
    return obj;
}



@end

