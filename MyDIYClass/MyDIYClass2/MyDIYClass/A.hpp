//
//  A.hpp
//  MyDIYClass2
//
//  Created by kyson on 2019/6/27.
//  Copyright © 2019 cn.kyson. All rights reserved.
//

#ifndef A_hpp
#define A_hpp

#include <stdio.h>
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



#endif /* A_hpp */
