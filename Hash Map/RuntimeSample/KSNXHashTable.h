//
//  KSNXHashTable.h
//  Algorithm
//
//  Created by ronglei on 15/7/2.
//  Copyright (c) 2015年 ronglei. All rights reserved.
//

#ifndef __Algorithm__KSNXHashTable__
#define __Algorithm__KSNXHashTable__

#include <stdio.h>
#include <stdbool.h>

#if !defined(OBJC_EXTERN)
#   if defined(__cplusplus)
#       define OBJC_EXTERN extern "C"
#   else
#       define OBJC_EXTERN extern
#   endif
#endif

#if !defined(OBJC_VISIBLE)
#   if TARGET_OS_WIN32
#       if defined(BUILDING_OBJC)
#           define OBJC_VISIBLE __declspec(dllexport)
#       else
#           define OBJC_VISIBLE __declspec(dllimport)
#       endif
#   else
#       define OBJC_VISIBLE  __attribute__((visibility("default")))
#   endif
#endif

#if !defined(OBJC_EXPORT)
#   define OBJC_EXPORT  OBJC_EXTERN OBJC_VISIBLE
#endif

/**
 * hash     :决定了获取节点index的计算规则
 * isEqual  :用于判断两个节点是否相同
 * free     :用于释放节点
 */
typedef struct {
    uintptr_t	(*hash)(const void *info, const void *data);
    int         (*isEqual)(const void *info, const void *data1, const void *data2);
    void        (*free)(const void *info, void *data);
    int         style; /* reserved for future expansion; currently 0 */
} KSNXHashTablePrototype;

/**
 * hash表的元素需要满足两种约束：
 * 1.相同的data值应该具有相同的hash值,即data1 = data2 -> hash(data1) = hash(data2)
 * 2.isEqual函数可以确定两个data是否相同,即isEqual(data1, data2) == 1 -> data1 = data2
 */

/**
 * 下面是hash结构体
 * prototype  :hash函数表
 * count      :hash结构已经有的key个数
 * nbBuckets  :hash表的容量大小,随count增多而改变
 * 当count > nbBuckets时会通过_NXHashRehash函数进行扩扩容
 * buckets    :hash表的数组基址
 */
typedef struct {
    const KSNXHashTablePrototype	*prototype;
    unsigned                    count;
    unsigned                    nbBuckets;
    void                        *buckets;
    const void                  *info;
} KSNXHashTable;

OBJC_EXPORT KSNXHashTable *KSKSNXCreateHashTableFromZone(KSNXHashTablePrototype prototype, unsigned capacity, const void *info, void *z);
OBJC_EXPORT KSNXHashTable *KSNXCreateHashTable(KSNXHashTablePrototype prototype, unsigned capacity, const void *info);

OBJC_EXPORT void KSNXFreeHashTable(KSNXHashTable *table);
OBJC_EXPORT void KSNXEmptyHashTable(KSNXHashTable *table);
OBJC_EXPORT void KSNXResetHashTable(KSNXHashTable *table);
OBJC_EXPORT bool KSNXCompareHashTables(KSNXHashTable *table1, KSNXHashTable *table2);
OBJC_EXPORT KSNXHashTable *KSNXCopyHashTable(KSNXHashTable *table);
OBJC_EXPORT unsigned KSNXCountHashTable(KSNXHashTable *table);
OBJC_EXPORT int KSNXHashMember(KSNXHashTable *table, const void *data);
OBJC_EXPORT void *KSNXHashGet(KSNXHashTable *table, const void *data);
OBJC_EXPORT void *KSNXHashInsert(KSNXHashTable *table, const void *data);
OBJC_EXPORT void *KSNXHashInsertIfAbsent(KSNXHashTable *table, const void *data);
OBJC_EXPORT void *KSNXHashRemove(KSNXHashTable *table, const void *data);

typedef struct {
    int i;
    int j;
} KSNXHashState;

KSNXHashState KSNXInitHashState(KSNXHashTable *table);
OBJC_EXPORT int KSNXNextHashState(KSNXHashTable *table, KSNXHashState *state, void **data);
uintptr_t KSNXPtrHash(const void *info, const void *data);
uintptr_t KSNXStrHash(const void *info, const void *data);
int KSNXPtrIsEqual(const void *info, const void *data1, const void *data2);
int KSNXStrIsEqual(const void *info, const void *data1, const void *data2);
void KSNXNoEffectFree(const void *info, void *data);
void KSNXReallyFree(const void *info, void *data);

OBJC_EXPORT const KSNXHashTablePrototype KSNXPtrPrototype;
OBJC_EXPORT const KSNXHashTablePrototype KSNXStrPrototype;

OBJC_EXPORT const KSNXHashTablePrototype KSNXPtrStructKeyPrototype;
OBJC_EXPORT const KSNXHashTablePrototype KSNXStrStructKeyPrototype;

#if !__OBJC2__ && !TARGET_OS_WIN32

typedef const char *NXAtom;
NXAtom NXUniqueString(const char *buffer);
NXAtom NXUniqueStringWithLength(const char *buffer, int length);
NXAtom NXUniqueStringNoCopy(const char *string);
char *NXCopyStringBuffer(const char *buffer);
char *NXCopyStringBufferFromZone(const char *buffer, void *z);

#endif

#endif
