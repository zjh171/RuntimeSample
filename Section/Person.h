//
//  Person.hpp
//  SectionDemo
//
//  Created by kyson on 2021/4/4.
//

#ifndef Person_hpp
#define Person_hpp

#include <stdio.h>

class Person{
public:
    Person(){
        printf("Person::Person()\n");
    }

    ~Person(){
        printf("Person::~Person()\n");
    }
};


class iOSer : Person{
public:
    iOSer(){
        printf("iOSer::iOSer()\n");
    }

    ~iOSer(){
        printf("iOSer::~iOSer()\n");
    }
};

#endif /* Person_hpp */
