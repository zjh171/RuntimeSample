//
//  KSNXMapTable.h
//  Algorithm
//
//  Created by ronglei on 15/7/7.
//  Copyright (c) 2015年 ronglei. All rights reserved.
//

#ifndef __Algorithm__KSNXMapTable__
#define __Algorithm__KSNXMapTable__

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


/* This module allows hashing of arbitrary associations [key -> value].  Keys and values must be pointers or integers, and client is responsible for allocating/deallocating this data.  A deallocation call-back is provided.
 NX_MAPNOTAKEY (-1) is used internally as a marker, and therefore keys must always be different from -1.
 As well-behaved scalable data structures, hash tables double in size when they start becoming full, thus guaranteeing both average constant time access and linear size. */

typedef struct _KSNXMapTable {
    /* private data structure; may change */
    const struct    _KSNXMapTablePrototype	*prototype;
    unsigned        count;
    unsigned        nbBucketsMinusOne;
    void            *buckets;
} KSNXMapTable;

typedef struct _KSNXMapTablePrototype {
    unsigned    (*hash)(KSNXMapTable *, const void *key);
    int         (*isEqual)(KSNXMapTable *, const void *key1, const void *key2);
    void        (*free)(KSNXMapTable *, void *key, void *value);
    int         style; /* reserved for future expansion; currently 0 */
} KSNXMapTablePrototype;

/* invariants assumed by the implementation:
	A - key != -1
	B - key1 == key2 => hash(key1) == hash(key2)
 when key varies over time, hash(key) must remain invariant
 e.g. if string key, the string must not be changed
	C - isEqual(key1, key2) => key1 == key2
 */

#define NX_MAPNOTAKEY	((void *)(-1))

KSNXMapTable *NXCreateMapTableFromZone(KSNXMapTablePrototype prototype, unsigned capacity, void *z);
KSNXMapTable *NXCreateMapTable(KSNXMapTablePrototype prototype, unsigned capacity);
/* capacity is only a hint; 0 creates a small table */

void NXFreeMapTable(KSNXMapTable *table);
/* call free for each pair, and recovers table */

void NXResetMapTable(KSNXMapTable *table);
/* free each pair; keep current capacity */

bool NXCompareMapTables(KSNXMapTable *table1, KSNXMapTable *table2);
/* Returns YES if the two sets are equal (each member of table1 in table2, and table have same size) */

unsigned NXCountMapTable(KSNXMapTable *table);
/* current number of data in table */

void *NXMapMember(KSNXMapTable *table, const void *key, void **value);
/* return original table key or NX_MAPNOTAKEY.  If key is found, value is set */

void *NXMapGet(KSNXMapTable *table, const void *key);
/* return original corresponding value or NULL.  When NULL need be stored as value, NXMapMember can be used to test for presence */

void *NXMapInsert(KSNXMapTable *table, const void *key, const void *value);
/* override preexisting pair; Return previous value or NULL. */

void *NXMapRemove(KSNXMapTable *table, const void *key);
/* previous value or NULL is returned */

/* Iteration over all elements of a table consists in setting up an iteration state and then to progress until all entries have been visited.  An example of use for counting elements in a table is:
 unsigned	count = 0;
 const MyKey	*key;
 const MyValue	*value;
 NXMapState	state = NXInitMapState(table);
 while(NXNextMapState(table, &state, &key, &value)) {
	count++;
 }
 */

typedef struct {int index;} NXMapState;
/* callers should not rely on actual contents of the struct */

NXMapState NXInitMapState(KSNXMapTable *table);

int NXNextMapState(KSNXMapTable *table, NXMapState *state, const void **key, const void **value);
/* returns 0 when all elements have been visited */

/***************	Conveniences		***************/

OBJC_EXPORT const KSNXMapTablePrototype NXPtrValueMapPrototype;
/* hashing is pointer/integer hashing;
 isEqual is identity;
 free is no-op. */
OBJC_EXPORT const KSNXMapTablePrototype NXStrValueMapPrototype;
/* hashing is string hashing;
 isEqual is strcmp;
 free is no-op. */
OBJC_EXPORT const KSNXMapTablePrototype NXObjectMapPrototype;
/* for objects; uses methods: hash, isEqual:, free, all for key. */

#endif /* defined(__Algorithm__KSNXMapTable__) */

