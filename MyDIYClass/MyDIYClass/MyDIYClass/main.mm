//
//  main.m
//  MyDIYClass
//
//  Created by kyson on 2019/5/26.
//  Copyright © 2019 cn.kyson. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <dlfcn.h>
#include <mach-o/loader.h>
#include <mach-o/getsect.h>

struct class_ro_t1 {
    uint32_t flags;
    uint32_t instanceStart;
    uint32_t instanceSize;
#ifdef __LP64__
    uint32_t reserved;
#endif
    const uint8_t * ivarLayout;
    const char * name;
};

struct class_rw_t1 {
    // Be warned that Symbolication knows the layout of this structure.
    uint32_t flags;
    uint32_t version;
    const class_ro_t1 *ro;
    Class firstSubclass;
    Class nextSiblingClass;
    char *demangledName;
};

#if !__LP64__
#define FAST_DATA_MASK        0xfffffffcUL
#elif 1
#define FAST_DATA_MASK          0x00007ffffffffff8UL
#else
#define FAST_DATA_MASK          0x00007ffffffffff8UL
#endif

struct class_data_bits_t {
    
    // Values are the FAST_ flags above.
    uintptr_t bits;
private:
    bool getBit(uintptr_t bit)
    {
        return bits & bit;
    }
    
public:
    
    class_rw_t1* data() {
        return (class_rw_t1 *)(bits & FAST_DATA_MASK);
    }
    
};


#if __LP64__
typedef uint32_t mask_t;  // x86_64 & arm64 asm are less efficient with 16-bits
#else
typedef uint16_t mask_t;
#endif

struct cache_t1 {
    struct bucket_t *_buckets;
    mask_t _mask;
    mask_t _occupied;
    
public:
    struct bucket_t *buckets();
    mask_t mask();
    mask_t occupied();
    void incrementOccupied();
    void setBucketsAndMask(struct bucket_t *newBuckets, mask_t newMask);
    void initializeToEmpty();
    
    mask_t capacity();
    bool isConstantEmptyCache();
    bool canBeFreed();
    
    static size_t bytesForCapacity(uint32_t cap);
    static struct bucket_t * endMarker(struct bucket_t *b, uint32_t cap);
    
    void expand();
    void reallocate(mask_t oldCapacity, mask_t newCapacity);
    //    struct bucket_t * find(cache_key_t key, id receiver);
    //
    static void bad_cache(id receiver, SEL sel, Class isa) __attribute__((noreturn));
};

struct objc_class1 : objc_object {
    // Class ISA;
    Class superclass;
    cache_t1 cache;             // formerly cache pointer and vtable
    class_data_bits_t bits;    // class_rw_t * plus custom rr/alloc flags
    
    class_rw_t1 *data() {
        return bits.data();
    }
    
    const char *mangledName() {
        return ((const class_ro_t1 *)data())->name;
    }
    
    const char *demangledName(bool realize = false);
    const char *nameForLogging();
};

typedef struct objc_class1 *Class1;
typedef struct classref * classref_t;


#ifndef __LP64__
#define mach_header mach_header
#else
#define mach_header mach_header_64
#endif


const struct mach_header *machHeader = NULL;
static NSString *configuration = @"";

int main()
{
    unsigned long byteCount = 0;
    
    if (machHeader == NULL)
    {
        Dl_info info;
        dladdr((__bridge const void *)(configuration), &info);
        machHeader = (struct mach_header_64*)info.dli_fbase;
    }
    
    uintptr_t* data = (uintptr_t *) getsectiondata(machHeader, "__DATA", "__objc_classlist", &byteCount);
    
    NSUInteger counter = byteCount/sizeof(void*);
    
    for(NSUInteger idx = 0; idx < counter; ++idx)
    {
        Class1 cls =(Class1)( data[idx]);
        NSLog(@"class:%s",cls->mangledName());
    }
    
    
    return 0;
}
