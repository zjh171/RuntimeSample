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




// class_data_bits_t is the class_t->data field (class_rw_t pointer plus flags)
// The extra bits are optimized for the retain/release and alloc/dealloc paths.

// Values for class_ro_t->flags
// These are emitted by the compiler and are part of the ABI.
// Note: See CGObjCNonFragileABIMac::BuildClassRoTInitializer in clang
// class is a metaclass
#define RO_META               (1<<0)
// class is a root class
#define RO_ROOT               (1<<1)
// class has .cxx_construct/destruct implementations
#define RO_HAS_CXX_STRUCTORS  (1<<2)
// class has +load implementation
// #define RO_HAS_LOAD_METHOD    (1<<3)
// class has visibility=hidden set
#define RO_HIDDEN             (1<<4)
// class has attribute(objc_exception): OBJC_EHTYPE_$_ThisClass is non-weak
#define RO_EXCEPTION          (1<<5)
// this bit is available for reassignment
// #define RO_REUSE_ME           (1<<6)
// class compiled with ARC
#define RO_IS_ARC             (1<<7)
// class has .cxx_destruct but no .cxx_construct (with RO_HAS_CXX_STRUCTORS)
#define RO_HAS_CXX_DTOR_ONLY  (1<<8)
// class is not ARC but has ARC-style weak ivar layout
#define RO_HAS_WEAK_WITHOUT_ARC (1<<9)

// class is in an unloadable bundle - must never be set by compiler
#define RO_FROM_BUNDLE        (1<<29)
// class is unrealized future class - must never be set by compiler
#define RO_FUTURE             (1<<30)
// class is realized - must never be set by compiler
#define RO_REALIZED           (1<<31)

// Values for class_rw_t->flags
// These are not emitted by the compiler and are never used in class_ro_t.
// Their presence should be considered in future ABI versions.
// class_t->data is class_rw_t, not class_ro_t
#define RW_REALIZED           (1<<31)
// class is unresolved future class
#define RW_FUTURE             (1<<30)
// class is initialized
#define RW_INITIALIZED        (1<<29)
// class is initializing
#define RW_INITIALIZING       (1<<28)
// class_rw_t->ro is heap copy of class_ro_t
#define RW_COPIED_RO          (1<<27)
// class allocated but not yet registered
#define RW_CONSTRUCTING       (1<<26)
// class allocated and registered
#define RW_CONSTRUCTED        (1<<25)
// available for use; was RW_FINALIZE_ON_MAIN_THREAD
// #define RW_24 (1<<24)
// class +load has been called
#define RW_LOADED             (1<<23)
#if !SUPPORT_NONPOINTER_ISA
// class instances may have associative references
#define RW_INSTANCES_HAVE_ASSOCIATED_OBJECTS (1<<22)
#endif
// class has instance-specific GC layout
#define RW_HAS_INSTANCE_SPECIFIC_LAYOUT (1 << 21)
// available for use
// #define RW_20       (1<<20)
// class has started realizing but not yet completed it
#define RW_REALIZING          (1<<19)

// NOTE: MORE RW_ FLAGS DEFINED BELOW


// Values for class_rw_t->flags or class_t->bits
// These flags are optimized for retain/release and alloc/dealloc
// 64-bit stores more of them in class_t->bits to reduce pointer indirection.

#if !__LP64__

// class or superclass has .cxx_construct implementation
#define RW_HAS_CXX_CTOR       (1<<18)
// class or superclass has .cxx_destruct implementation
#define RW_HAS_CXX_DTOR       (1<<17)
// class or superclass has default alloc/allocWithZone: implementation
// Note this is is stored in the metaclass.
#define RW_HAS_DEFAULT_AWZ    (1<<16)
// class's instances requires raw isa
#if SUPPORT_NONPOINTER_ISA
#define RW_REQUIRES_RAW_ISA   (1<<15)
#endif

// class is a Swift class
#define FAST_IS_SWIFT         (1UL<<0)
// class or superclass has default retain/release/autorelease/retainCount/
//   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
#define FAST_HAS_DEFAULT_RR   (1UL<<1)
// data pointer
#define FAST_DATA_MASK        0xfffffffcUL

#elif 1
// Leaks-compatible version that steals low bits only.

// class or superclass has .cxx_construct implementation
#define RW_HAS_CXX_CTOR       (1<<18)
// class or superclass has .cxx_destruct implementation
#define RW_HAS_CXX_DTOR       (1<<17)
// class or superclass has default alloc/allocWithZone: implementation
// Note this is is stored in the metaclass.
#define RW_HAS_DEFAULT_AWZ    (1<<16)

// class is a Swift class
#define FAST_IS_SWIFT           (1UL<<0)
// class or superclass has default retain/release/autorelease/retainCount/
//   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
#define FAST_HAS_DEFAULT_RR     (1UL<<1)
// class's instances requires raw isa
#define FAST_REQUIRES_RAW_ISA   (1UL<<2)
// data pointer
#define FAST_DATA_MASK          0x00007ffffffffff8UL

#else
// Leaks-incompatible version that steals lots of bits.

// class is a Swift class
#define FAST_IS_SWIFT           (1UL<<0)
// class's instances requires raw isa
#define FAST_REQUIRES_RAW_ISA   (1UL<<1)
// class or superclass has .cxx_destruct implementation
//   This bit is aligned with isa_t->hasCxxDtor to save an instruction.
#define FAST_HAS_CXX_DTOR       (1UL<<2)
// data pointer
#define FAST_DATA_MASK          0x00007ffffffffff8UL
// class or superclass has .cxx_construct implementation
#define FAST_HAS_CXX_CTOR       (1UL<<47)
// class or superclass has default alloc/allocWithZone: implementation
// Note this is is stored in the metaclass.
#define FAST_HAS_DEFAULT_AWZ    (1UL<<48)
// class or superclass has default retain/release/autorelease/retainCount/
//   _tryRetain/_isDeallocating/retainWeakReference/allowsWeakReference
#define FAST_HAS_DEFAULT_RR     (1UL<<49)
// summary bit for fast alloc path: !hasCxxCtor and
//   !instancesRequireRawIsa and instanceSize fits into shiftedSize
#define FAST_ALLOC              (1UL<<50)
// instance size in units of 16 bytes
//   or 0 if the instance size is too big in this field
//   This field must be LAST
#define FAST_SHIFTED_SIZE_SHIFT 51

// FAST_ALLOC means
//   FAST_HAS_CXX_CTOR is set
//   FAST_REQUIRES_RAW_ISA is not set
//   FAST_SHIFTED_SIZE is not zero
// FAST_ALLOC does NOT check FAST_HAS_DEFAULT_AWZ because that
// bit is stored on the metaclass.
#define FAST_ALLOC_MASK  (FAST_HAS_CXX_CTOR | FAST_REQUIRES_RAW_ISA)
#define FAST_ALLOC_VALUE (0)

#endif


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

typedef struct objc_class1 *Class1;
typedef struct classref * classref_t;

struct objc_class1 : objc_object {
    // Class ISA;
    Class1 superclass;
    cache_t1 cache;             // formerly cache pointer and vtable
    class_data_bits_t bits;    // class_rw_t * plus custom rr/alloc flags
    
    class_rw_t1 *data() {
        return bits.data();
    }
    
    const char *mangledName1() {
        return ((const class_ro_t1 *)data())->name;
    }
    
    const char *demangledName(bool realize = false);
    const char *nameForLogging();
};



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
        NSLog(@"class:%s \n",cls->mangledName1());
        
        bool isRoot = (cls->data()->flags & RO_ROOT);
        if (isRoot) {
            printf("is root \n");
        } else {
            printf("is not root \n");
            
            Class1 superClass = cls->superclass;
            printf("superClass:%s \n",superClass->mangledName1());
        }
        
        
        bool isRealized = cls->data()->flags & RW_REALIZED;
        if (isRealized) {
            printf("is isRealized\n");
        } else {
            printf("is not Realized\n");
        }
        
        
        bool isARC = cls->data()->flags & RO_IS_ARC;
        if (isARC) {
            printf("is arc\n");
        } else {
            printf("is  not arc\n");
        }
        
        
        
        bool loadMethodHasCalled = cls->data()->flags & RW_LOADED;
        if (loadMethodHasCalled) {
            printf("loadMethod Has Called \n");
        } else {
            printf("loadMethod has not Called \n");
        }
        
        
        bool hasCXXCtor = cls->data()->flags & RO_HAS_CXX_STRUCTORS;
        if (hasCXXCtor) {
            printf(" Has HAS_CXX_CTOR \n");
        } else {
            printf(" has not HAS_CXX_CTOR \n");
        }
        
        bool hasWeakWithoutARC = cls->data()->flags & RO_HAS_WEAK_WITHOUT_ARC;
        if (hasWeakWithoutARC) {
            printf(" Has RO_HAS_WEAK_WITHOUT_ARC \n");
        } else {
            printf(" has not RO_HAS_WEAK_WITHOUT_ARC \n");
        }
        
        
        bool hasRealized = cls->data()->flags & RO_REALIZED;
        if (hasRealized) {
            printf(" Has hasRealized \n");
        } else {
            printf(" has not hasRealized \n");
        }
        
    }
    
    return 0;
}
