//
//  KSNXMapTable.c
//  Algorithm
//
//  Created by ronglei on 15/7/7.
//  Copyright (c) 2015年 ronglei. All rights reserved.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc/malloc.h>

#include "KSNXMapTable.h"
#include "KSNXHashTable.h"

#define INLINE inline

/***********************************************************************
 * _objc_internal_zone.
 * Malloc zone for internal runtime data.
 * By default this is the default malloc zone, but a dedicated zone is
 * used if environment variable OBJC_USE_INTERNAL_ZONE is set.
 **********************************************************************/
malloc_zone_t *_objc_internal_zone(void)
{
    static malloc_zone_t *z = (malloc_zone_t *)-1;
    if (z == (malloc_zone_t *)-1) {
        z = malloc_default_zone();
    }
    return z;
}

char *_strdup_internal(const char *str)
{
    size_t len;
    char *dup;
    if (!str) return NULL;
    len = strlen(str);
    dup = (char *)malloc_zone_malloc(_objc_internal_zone(), len + 1);
    memcpy(dup, str, len + 1);
    return dup;
}

void _free_internal(void *ptr)
{
    malloc_zone_free(_objc_internal_zone(), ptr);
}


/******		Macros and utilities	****************************/

typedef struct _MapPair {
    const void	*key;
    const void	*value;
} MapPair;

static unsigned log2u(unsigned x) { return (x<2) ? 0 : log2u(x>>1)+1; };

static INLINE unsigned exp2u(unsigned x) { return (1 << x); };

static INLINE unsigned xorHash(unsigned hash)
{
    unsigned xored = (hash & 0xffff) ^ (hash >> 16);
    return ((xored * 65521) + hash);
}

static INLINE unsigned bucketOf(KSNXMapTable *table, const void *key)
{
    unsigned	hash = (table->prototype->hash)(table, key);
    return hash & table->nbBucketsMinusOne;
}

static INLINE int isEqual(KSNXMapTable *table, const void *key1, const void *key2)
{
    return (key1 == key2) ? 1 : (table->prototype->isEqual)(table, key1, key2);
}

static INLINE unsigned nextIndex(KSNXMapTable *table, unsigned index)
{
    return (index + 1) & table->nbBucketsMinusOne;
}

static INLINE void *allocBuckets(void *z, unsigned nb)
{
    MapPair	*pairs = 1+(MapPair *)malloc_zone_malloc((malloc_zone_t *)z, ((nb+1) * sizeof(MapPair)));
    MapPair	*pair = pairs;
    while (nb--) { pair->key = NX_MAPNOTAKEY; pair->value = NULL; pair++; }
    
    return pairs;
}

static INLINE void freeBuckets(void *p) { free(-1+(MapPair *)p); }

/*****		Global data and bootstrap	**********************/

static int isEqualPrototype (const void *info, const void *data1, const void *data2)
{
    KSNXHashTablePrototype        *proto1 = (KSNXHashTablePrototype *) data1;
    KSNXHashTablePrototype        *proto2 = (KSNXHashTablePrototype *) data2;
    
    return (proto1->hash == proto2->hash) && (proto1->isEqual == proto2->isEqual) && (proto1->free == proto2->free) && (proto1->style == proto2->style);
};

static uintptr_t hashPrototype (const void *info, const void *data)
{
    KSNXHashTablePrototype        *proto = (KSNXHashTablePrototype *) data;
    
    return KSNXPtrHash(info, (void*)proto->hash) ^ KSNXPtrHash(info, (void*)proto->isEqual) ^ KSNXPtrHash(info, (void*)proto->free) ^ (uintptr_t) proto->style;
};

/**
 *  默认的prototype属性，如果调用NXCreateMapTableFromZone函数时不传递prototype出行就是用该protoPrototype
 *  libobjc中大部分hash的创建都不使用该prototype属性默认值
 */
static KSNXHashTablePrototype protoPrototype = { hashPrototype, isEqualPrototype, KSNXNoEffectFree, 0 };

static KSNXHashTable *prototypes = NULL;
/* table of all prototypes */

/****		Fundamentals Operations			**************/

KSNXMapTable *NXCreateMapTableFromZone(KSNXMapTablePrototype prototype, unsigned capacity, void *z)
{
    KSNXMapTable			*table = (KSNXMapTable *)malloc_zone_malloc((malloc_zone_t *)z, sizeof(KSNXMapTable));
    KSNXMapTablePrototype	*proto;
    if (! prototypes) prototypes = KSNXCreateHashTable(protoPrototype, 0, NULL);
    if (! prototype.hash || ! prototype.isEqual || ! prototype.free || prototype.style) {
        printf("*** NXCreateMapTable: invalid creation parameters\n");
        return NULL;
    }
    proto = (KSNXMapTablePrototype *)KSNXHashGet(prototypes, &prototype);
    if (! proto) {
        proto = (KSNXMapTablePrototype *)malloc(sizeof(KSNXMapTablePrototype));
        *proto = prototype;
        (void)KSNXHashInsert(prototypes, proto);
    }
    table->prototype = proto;
    table->count = 0;
    table->nbBucketsMinusOne = exp2u(log2u(capacity)+1) - 1;
    table->buckets = (MapPair *)allocBuckets(z, table->nbBucketsMinusOne + 1);
    
    return table;
}

KSNXMapTable *NXCreateMapTable(KSNXMapTablePrototype prototype, unsigned capacity)
{
    return NXCreateMapTableFromZone(prototype, capacity, malloc_default_zone());
}

void NXFreeMapTable(KSNXMapTable *table)
{
    NXResetMapTable(table);
    freeBuckets(table->buckets);
    free(table);
}

void NXResetMapTable(KSNXMapTable *table)
{
    MapPair	*pairs = (MapPair *)table->buckets;
    void	(*freeProc)(struct _KSNXMapTable *, void *, void *) = table->prototype->free;
    unsigned	index = table->nbBucketsMinusOne + 1;
    while (index--) {
        if (pairs->key != NX_MAPNOTAKEY) {
            freeProc(table, (void *)pairs->key, (void *)pairs->value);
            pairs->key = NX_MAPNOTAKEY; pairs->value = NULL;
        }
        pairs++;
    }
    table->count = 0;
}

bool NXCompareMapTables(KSNXMapTable *table1, KSNXMapTable *table2)
{
    if (table1 == table2) return true;
    if (table1->count != table2->count) return false;
    else {
        const void *key;
        const void *value;
        NXMapState	state = NXInitMapState(table1);
        while (NXNextMapState(table1, &state, &key, &value)) {
            if (NXMapMember(table2, key, (void**)&value) == NX_MAPNOTAKEY) return false;
        }
        return true;
    }
}

unsigned NXCountMapTable(KSNXMapTable *table) { return table->count; }

static INLINE void *_NXMapMember(KSNXMapTable *table, const void *key, void **value)
{
    MapPair	*pairs = (MapPair *)table->buckets;
    unsigned	index = bucketOf(table, key);
    MapPair	*pair = pairs + index;
    if (pair->key == NX_MAPNOTAKEY) return NX_MAPNOTAKEY;
    if (isEqual(table, pair->key, key)) {
        *value = (void *)pair->value;
        return (void *)pair->key;
    } else {
        unsigned	index2 = index;
        while ((index2 = nextIndex(table, index2)) != index) {
            pair = pairs + index2;
            if (pair->key == NX_MAPNOTAKEY) return NX_MAPNOTAKEY;
            if (isEqual(table, pair->key, key)) {
                *value = (void *)pair->value;
                return (void *)pair->key;
            }
        }
        return NX_MAPNOTAKEY;
    }
}

void *NXMapMember(KSNXMapTable *table, const void *key, void **value)
{
    return _NXMapMember(table, key, value);
}

void *NXMapGet(KSNXMapTable *table, const void *key)
{
    void	*value;
    return (_NXMapMember(table, key, &value) != NX_MAPNOTAKEY) ? value : NULL;
}

static void _NXMapRehash(KSNXMapTable *table)
{
    MapPair	*pairs = (MapPair *)table->buckets;
    MapPair	*pair = pairs;
    unsigned	numBuckets = table->nbBucketsMinusOne + 1;
    unsigned	index = numBuckets;
    unsigned	oldCount = table->count;
    
    table->nbBucketsMinusOne = 2 * numBuckets - 1;
    table->count = 0;
    table->buckets = allocBuckets(malloc_zone_from_ptr(table), table->nbBucketsMinusOne + 1);
    while (index--) {
        if (pair->key != NX_MAPNOTAKEY) {
            (void)NXMapInsert(table, pair->key, pair->value);
        }
        pair++;
    }
    if (oldCount != table->count)
        printf("*** maptable: count differs after rehashing; probably indicates a broken invariant: there are x and y such as isEqual(x, y) is TRUE but hash(x) != hash (y)\n");
    freeBuckets(pairs);
}

void *NXMapInsert(KSNXMapTable *table, const void *key, const void *value)
{
    MapPair	*pairs = (MapPair *)table->buckets;
    unsigned index = bucketOf(table, key);
    MapPair	*pair = pairs + index;
    if (key == NX_MAPNOTAKEY) {
        printf("*** NXMapInsert: invalid key: -1\n");
        return NULL;
    }
    
    unsigned numBuckets = table->nbBucketsMinusOne + 1;
    
    if (pair->key == NX_MAPNOTAKEY) {
        pair->key = key; pair->value = value;
        table->count++;
        if (table->count * 4 > numBuckets * 3) _NXMapRehash(table);
        return NULL;
    }
    
    if (isEqual(table, pair->key, key)) {
        const void	*old = pair->value;
        if (old != value) pair->value = value;/* avoid writing unless needed! */
        return (void *)old;
    } else if (table->count == numBuckets) {
        /* no room: rehash and retry */
        _NXMapRehash(table);
        return NXMapInsert(table, key, value);
    } else {
        unsigned	index2 = index;
        while ((index2 = nextIndex(table, index2)) != index) {
            pair = pairs + index2;
            if (pair->key == NX_MAPNOTAKEY) {
                pair->key = key; pair->value = value;
                table->count++;
                if (table->count * 4 > numBuckets * 3) _NXMapRehash(table);
                return NULL;
            }
            if (isEqual(table, pair->key, key)) {
                const void	*old = pair->value;
                if (old != value) pair->value = value;/* avoid writing unless needed! */
                return (void *)old;
            }
        }
        /* no room: can't happen! */
        printf("**** NXMapInsert: bug\n");
        return NULL;
    }
}

static int mapRemove = 0;

void *NXMapRemove(KSNXMapTable *table, const void *key)
{
    MapPair	*pairs = (MapPair *)table->buckets;
    unsigned	index = bucketOf(table, key);
    MapPair	*pair = pairs + index;
    unsigned	chain = 1; /* number of non-nil pairs in a row */
    int		found = 0;
    const void	*old = NULL;
    if (pair->key == NX_MAPNOTAKEY) return NULL;
    mapRemove ++;
    /* compute chain */
    {
        unsigned	index2 = index;
        if (isEqual(table, pair->key, key)) {found ++; old = pair->value; }
        while ((index2 = nextIndex(table, index2)) != index) {
            pair = pairs + index2;
            if (pair->key == NX_MAPNOTAKEY) break;
            if (isEqual(table, pair->key, key)) {found ++; old = pair->value; }
            chain++;
        }
    }
    if (! found) return NULL;
    if (found != 1) printf("**** NXMapRemove: incorrect table\n");
    /* remove then reinsert */
    {
        MapPair	buffer[16];
        MapPair	*aux = (chain > 16) ? (MapPair *)malloc(sizeof(MapPair)*(chain-1)) : buffer;
        unsigned	auxnb = 0;
        int	nb = chain;
        unsigned	index2 = index;
        while (nb--) {
            pair = pairs + index2;
            if (! isEqual(table, pair->key, key)) aux[auxnb++] = *pair;
            pair->key = NX_MAPNOTAKEY; pair->value = NULL;
            index2 = nextIndex(table, index2);
        }
        table->count -= chain;
        if (auxnb != chain-1) printf("**** NXMapRemove: bug\n");
        while (auxnb--) NXMapInsert(table, aux[auxnb].key, aux[auxnb].value);
        if (chain > 16) free(aux);
    }
    return (void *)old;
}

NXMapState NXInitMapState(KSNXMapTable *table)
{
    NXMapState	state;
    state.index = table->nbBucketsMinusOne + 1;
    return state;
}

int NXNextMapState(KSNXMapTable *table, NXMapState *state, const void **key, const void **value)
{
    MapPair	*pairs = (MapPair *)table->buckets;
    while (state->index--) {
        MapPair	*pair = pairs + state->index;
        if (pair->key != NX_MAPNOTAKEY) {
            *key = pair->key; *value = pair->value;
            return 1;
        }
    }
    return 0;
}


/***********************************************************************
 * NXMapKeyCopyingInsert
 * Like NXMapInsert, but strdups the key if necessary.
 * Used to prevent stale pointers when bundles are unloaded.
 **********************************************************************/
void *NXMapKeyCopyingInsert(KSNXMapTable *table, const void *key, const void *value)
{
    void *realKey;
    void *realValue = NULL;
    
    if ((realKey = NXMapMember(table, key, &realValue)) != NX_MAPNOTAKEY) {
        // key DOES exist in table - use table's key for insertion
    } else {
        // key DOES NOT exist in table - copy the new key before insertion
        realKey = (void *)_strdup_internal((char *)key);
    }
    return NXMapInsert(table, realKey, value);
}


/***********************************************************************
 * NXMapKeyFreeingRemove
 * Like NXMapRemove, but frees the existing key if necessary.
 * Used to prevent stale pointers when bundles are unloaded.
 **********************************************************************/
void *NXMapKeyFreeingRemove(KSNXMapTable *table, const void *key)
{
    void *realKey;
    void *realValue = NULL;
    
    if ((realKey = NXMapMember(table, key, &realValue)) != NX_MAPNOTAKEY) {
        // key DOES exist in table - remove pair and free key
        realValue = NXMapRemove(table, realKey);
        _free_internal(realKey); // the key from the table, not necessarily the one given
        return realValue;
    } else {
        // key DOES NOT exist in table - nothing to do
        return NULL;
    }
}

/****		Conveniences		*************************************/

static unsigned _mapPtrHash(KSNXMapTable *table, const void *key)
{
#ifdef __LP64__
    return (unsigned)(((uintptr_t)key) >> 3);
#else
    return ((uintptr_t)key) >> 2;
#endif
}

static unsigned _mapStrHash(KSNXMapTable *table, const void *key)
{
    unsigned		hash = 0;
    unsigned char	*s = (unsigned char *)key;
    /* unsigned to avoid a sign-extend */
    /* unroll the loop */
    if (s) for (; ; ) {
        if (*s == '\0') break;
        hash ^= *s++;
        if (*s == '\0') break;
        hash ^= *s++ << 8;
        if (*s == '\0') break;
        hash ^= *s++ << 16;
        if (*s == '\0') break;
        hash ^= *s++ << 24;
    }
    return xorHash(hash);
}

static int _mapPtrIsEqual(KSNXMapTable *table, const void *key1, const void *key2)
{
    return key1 == key2;
}

static int _mapStrIsEqual(KSNXMapTable *table, const void *key1, const void *key2)
{
    if (key1 == key2) return 1;
    if (! key1) return ! strlen ((char *) key2);
    if (! key2) return ! strlen ((char *) key1);
    if (((char *) key1)[0] != ((char *) key2)[0]) return 0;
    return (strcmp((char *) key1, (char *) key2)) ? 0 : 1;
}

static void _mapNoFree(KSNXMapTable *table, void *key, void *value) {}
const KSNXMapTablePrototype NXPtrValueMapPrototype = { _mapPtrHash, _mapPtrIsEqual, _mapNoFree, 0 };
const KSNXMapTablePrototype NXStrValueMapPrototype = { _mapStrHash, _mapStrIsEqual, _mapNoFree, 0 };
