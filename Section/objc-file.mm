//
//  objc-file.cpp
//  SectionDemo
//
//  Created by kyson on 2021/4/4.
//

#include "objc-file.h"
#include "objc-private.h"


template <typename T>
T* getDataSection(const headerType *mhdr, const char *sectname,
                  size_t *outBytes, size_t *outCount)
{
    unsigned long byteCount = 0;
    T* data = (T*)getsectiondata(mhdr, "__DATA", sectname, &byteCount);
    if (!data) {
        data = (T*)getsectiondata(mhdr, "__DATA_CONST", sectname, &byteCount);
    }
    if (!data) {
        data = (T*)getsectiondata(mhdr, "__DATA_DIRTY", sectname, &byteCount);
    }
    if (outBytes) *outBytes = byteCount;
    if (outCount) *outCount = byteCount / sizeof(T);
    return data;
}


UnsignedInitializer *getLibsectionInitializers(const headerType *mhdr, size_t *outCount) {
    return getDataSection<UnsignedInitializer>(mhdr, "__mod_init_func", nil, outCount);
}

//UnsignedInitializer *getLibsectionInitializers(const header_info *hi, size_t *outCount) {
//    return getDataSection<UnsignedInitializer>(hi->mhdr(), "__mod_init_func", nil, outCount);
//}
