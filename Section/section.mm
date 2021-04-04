//
//  section.cpp
//  Section
//
//  Created by kyson on 2021/4/4.
//

#include "section.h"
#include "objc-private.h"

#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#include "Person.h"
#include "objc-file.h"

Person kyson;
//iOSer kimi;

void callGlobalObjInitialMethod(){
    size_t count;

    auto inits = getLibsectionInitializers(&_mh_dylib_header, &count);
    for (size_t i = 0; i < count; i++) {
        UnsignedInitializer init = inits[i];
        init();
    }
}
