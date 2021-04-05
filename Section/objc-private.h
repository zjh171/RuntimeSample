//
//  objc-private.h
//  Section
//
//  Created by kyson on 2021/4/4.
//



#ifndef objc_private_h
#define objc_private_h



#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>


typedef struct mach_header_64 headerType;

//typedef struct header_info {
//private:
//    // Note, this is no longer a pointer, but instead an offset to a pointer
//    // from this location.
//    intptr_t mhdr_offset;
//
//    // Note, this is no longer a pointer, but instead an offset to a pointer
//    // from this location.
//    intptr_t info_offset;
//
//    // Offset from this location to the non-lazy class list
//    intptr_t nlclslist_offset;
//    uintptr_t nlclslist_count;
//
//    // Offset from this location to the non-lazy category list
//    intptr_t nlcatlist_offset;
//    uintptr_t nlcatlist_count;
//
//    // Offset from this location to the category list
//    intptr_t catlist_offset;
//    uintptr_t catlist_count;
//
//    // Offset from this location to the category list 2
//    intptr_t catlist2_offset;
//    uintptr_t catlist2_count;
//
//    // Do not add fields without editing ObjCModernAbstraction.hpp
//public:
//
////    header_info_rw *getHeaderInfoRW() {
////        header_info_rw *preopt =
////            isPreoptimized() ? getPreoptimizedHeaderRW(this) : nil;
////        if (preopt) return preopt;
////        else return &rw_data[0];
////    }
////
//    const headerType *mhdr() const {
//        return (const headerType *)(((intptr_t)&mhdr_offset) + mhdr_offset);
//    }
////
////    void setmhdr(const headerType *mhdr) {
////        mhdr_offset = (intptr_t)mhdr - (intptr_t)&mhdr_offset;
////    }
////
////    const objc_image_info *info() const {
////        return (const objc_image_info *)(((intptr_t)&info_offset) + info_offset);
////    }
////
////    void setinfo(const objc_image_info *info) {
////        info_offset = (intptr_t)info - (intptr_t)&info_offset;
////    }
////
////    const classref_t *nlclslist(size_t *outCount) const;
////
////    void set_nlclslist(const void *list) {
////        nlclslist_offset = (intptr_t)list - (intptr_t)&nlclslist_offset;
////    }
////
////    category_t * const *nlcatlist(size_t *outCount) const;
////
////    void set_nlcatlist(const void *list) {
////        nlcatlist_offset = (intptr_t)list - (intptr_t)&nlcatlist_offset;
////    }
////
////    category_t * const *catlist(size_t *outCount) const;
////
////    void set_catlist(const void *list) {
////        catlist_offset = (intptr_t)list - (intptr_t)&catlist_offset;
////    }
////
////    category_t * const *catlist2(size_t *outCount) const;
////
////    void set_catlist2(const void *list) {
////        catlist2_offset = (intptr_t)list - (intptr_t)&catlist2_offset;
////    }
//
////    bool isLoaded() {
////        return getHeaderInfoRW()->getLoaded();
////    }
////
////    void setLoaded(bool v) {
////        getHeaderInfoRW()->setLoaded(v);
////    }
////
////    bool areAllClassesRealized() {
////        return getHeaderInfoRW()->getAllClassesRealized();
////    }
////
////    void setAllClassesRealized(bool v) {
////        getHeaderInfoRW()->setAllClassesRealized(v);
////    }
////
////    header_info *getNext() {
////        return getHeaderInfoRW()->getNext();
////    }
////
////    void setNext(header_info *v) {
////        getHeaderInfoRW()->setNext(v);
////    }
////
////    bool isBundle() {
////        return mhdr()->filetype == MH_BUNDLE;
////    }
////
////    const char *fname() const {
////        return dyld_image_path_containing_address(mhdr());
////    }
////
////    bool isPreoptimized() const;
////
////    bool hasPreoptimizedSelectors() const;
////
////    bool hasPreoptimizedClasses() const;
////
////    bool hasPreoptimizedProtocols() const;
////
////    bool hasPreoptimizedSectionLookups() const;
////
////#if !__OBJC2__
////    struct old_protocol **proto_refs;
////    struct objc_module *mod_ptr;
////    size_t              mod_count;
////#endif
//
//private:
//    // Images in the shared cache will have an empty array here while those
//    // allocated at run time will allocate a single entry.
//
//} header_info;




#endif /* objc_private_h */
