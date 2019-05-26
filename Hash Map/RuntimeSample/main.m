//
//  main.m
//  RuntimeSample
//
//  Created by kyson on 2019/3/26.
//  Copyright © 2019 cn.kyson. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "KSNXMapTable.h"
#import "KSNXHashTable.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        KSNXMapTable *maptable = NXCreateMapTable(NXPtrValueMapPrototype, 16);
        int a = 1000;
        int* p = &a ;
        int* a1 = (int *)NXMapGet(maptable, "kyson");
        printf("%p\n",a1);
        
        NXMapInsert(maptable, "kyson", p);
        
        int * b = (int *)NXMapGet(maptable, "kyson");
        int b1 = *b;
        printf("%i\n",b1);
        
        NXMapRemove(maptable,"kyson");
        int *c = (int *)NXMapGet(maptable, "kyson");
        printf("%p\n",c);
        
    }
    return 0;
}
