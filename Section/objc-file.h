//
//  objc-file.hpp
//  SectionDemo
//
//  Created by kyson on 2021/4/4.
//

#ifndef objc_file_hpp
#define objc_file_hpp

#include "objc-private.h"

#ifndef nil
# if __has_feature(cxx_nullptr)
#   define nil nullptr
# else
#   define nil __DARWIN_NULL
# endif
#endif


#include <stdio.h>


#define ptrauth_sign_unauthenticated(__value, __key, __data) __value

// FIXME: rdar://29241917&33734254 clang doesn't sign static initializers.
struct UnsignedInitializer {
private:
    uintptr_t storage;
public:
    UnsignedInitializer(uint32_t offset) {
        storage = (uintptr_t)&_mh_dylib_header + offset;
    }

    void operator () () const {
        using Initializer = void(*)();
        Initializer init =
            ptrauth_sign_unauthenticated((Initializer)storage,
                                         ptrauth_key_function_pointer, 0);
        init();
    }
};


extern UnsignedInitializer* getLibsectionInitializers(const headerType *mhdr, size_t *count);


#endif /* objc_file_hpp */
