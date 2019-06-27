//
//  TestObject.h
//  debug-objc
//
//  Created by kyson on 2019/1/30.
//

#import <Foundation/Foundation.h>
#include <stdio.h>

extern int abcdefg;

NS_ASSUME_NONNULL_BEGIN



class A
{
public:
    //默认构造函数
    A()
    {
        num=1001;
        age=18;
    }
    //初始化构造函数
    A(int n,int a):num(n),age(a){}
private:
    int num;
    int age;
};




@interface TestObject : NSObject {
//    __weak NSObject *propertyA;
//    A a;
}

@end

NS_ASSUME_NONNULL_END
