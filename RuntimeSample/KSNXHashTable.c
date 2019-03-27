//
//  KSNXHashTable.c
//  Algorithm
//
//  Created by ronglei on 15/7/2.
//  Copyright (c) 2015年 ronglei. All rights reserved.
//

#include "KSNXHashTable.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <objc/objc-api.h>
#include <sys/_pthread/_pthread_mutex_t.h>

/**
 * 为了提高效率,使用以下结构
 * 当只有一个元素的时候 one就指代data的值
 * 当有多于一个元素的时候 就需要通过遍历many数组 来获取data的值
 */
typedef union {
    const void	*one;
    const void	**many;
} oneOrMany;

/**
 * hash表的元素,存储着具有相同hash值的所有key
 */
typedef struct	{
    unsigned 	count;
    oneOrMany	elements;
} HashBucket;


static unsigned log2u(unsigned x) { return (x<2) ? 0 : log2u (x>>1)+1; };
static unsigned exp2m1(unsigned x) { return (1 << x) - 1; };

#define	PTRSIZE             sizeof(void *)

#define	DEFAULT_ZONE        NULL
#define	ZONE_FROM_PTR(p)    NULL
#define	ALLOCTABLE(z)       ((KSNXHashTable *) malloc(sizeof (KSNXHashTable)))
#define	ALLOCBUCKETS(z,nb)  ((HashBucket *) calloc(nb, sizeof (HashBucket)))
#define	ALLOCPAIRS(z,nb)    (1+(const void **) calloc(nb+1, sizeof (void *)))
#define	FREEPAIRS(p)        (free((void*)(-1+p)))


/*  BUCKETOF是一个宏，它返回指定data值经过hash后的数组地址
 主要分三步:
 1.(*table->prototype->hash)(table->info,data)表示调用具体hash表的函数表的hash函数。对于ptr型的实际调用是KSNXPtrHash函数。
 2.((*table->prototype->hash)(table->info,data) % table->nbBuckets) 表示将得到的hash值进行求余，以对应于hash桶的索引。
 3.(((HashBucket*)table->buckets)+((*table->prototype->hash)(table->info, data) %table->nbBuckets))表示将索引加上基地址buckets，即时对应的data散列到的HashBucket数组中
 */
#define	BUCKETOF(table, data)   (((HashBucket *)table->buckets) + ((*table->prototype->hash)(table->info, data) % table->nbBuckets))
/* GOOD_CAPACITY返回恰当的buckets的容量大小，初始化函数KSKSNXCreateHashTableFromZone中会用到，比如：
 * c值       返回值
 * 0-1       1
 * 2-3       3
 * 4-7       7
 * 8-15      15
 * 16-31     31
 * 32-63     63
 * 64-127    127 ...
**/
#define GOOD_CAPACITY(c)        (exp2m1 (log2u (c)+1))
/*
 * MORE_CAPACITY根据当前容量返回扩容后的容量大小，在扩容函数_NXHashRehash中会用到
 **/
#define MORE_CAPACITY(b)        (b*2+1)

#define ISEQUAL(table, data1, data2) ((data1 == data2) || (*table->prototype->isEqual)(table->info, data1, data2))

/*************************************************************************
 *
 *	Global data and bootstrap
 *
 *************************************************************************/

static uintptr_t hashPrototype (const void *info, const void *data)
{
    KSNXHashTablePrototype *proto = (KSNXHashTablePrototype *)data;
    
    return KSNXPtrHash(info, (void*)proto->hash) ^ KSNXPtrHash(info, (void*)proto->isEqual) ^ KSNXPtrHash(info, (void*)proto->free) ^ (uintptr_t) proto->style;
};

static int isEqualPrototype (const void *info, const void *data1, const void *data2)
{
    KSNXHashTablePrototype *proto1 = (KSNXHashTablePrototype *)data1;
    KSNXHashTablePrototype *proto2 = (KSNXHashTablePrototype *)data2;
    
    return (proto1->hash == proto2->hash) && (proto1->isEqual == proto2->isEqual) && (proto1->free == proto2->free) && (proto1->style == proto2->style);
};

void KSNXNoEffectFree(const void *info, void *data) {};

/**
 *  初始化静态结构 static KSNXHashTablePrototype protoPrototype
 */
static KSNXHashTablePrototype protoPrototype = { hashPrototype, isEqualPrototype, KSNXNoEffectFree, 0 };

static KSNXHashTable *prototypes = NULL;

/**
 *  初始化静态结构体指针 static KSNXHashTable *prototypes
 */
static void bootstrap(void)
{
    free(malloc(8));
    prototypes = ALLOCTABLE (DEFAULT_ZONE);
    prototypes->prototype = &protoPrototype;
    prototypes->count = 1;
    prototypes->nbBuckets = 1; /* has to be 1 so that the right bucket is 0 */
    prototypes->buckets = ALLOCBUCKETS(DEFAULT_ZONE, 1);
    prototypes->info = NULL;
    ((HashBucket *) prototypes->buckets)[0].count = 1;
    ((HashBucket *) prototypes->buckets)[0].elements.one = &protoPrototype;
};

int KSNXPtrIsEqual (const void *info, const void *data1, const void *data2)
{
    return data1 == data2;
};

KSNXHashTable *KSNXCreateHashTable (KSNXHashTablePrototype prototype, unsigned capacity, const void *info)
{
    return KSKSNXCreateHashTableFromZone(prototype, capacity, info, DEFAULT_ZONE);
}

KSNXHashTable *KSKSNXCreateHashTableFromZone (KSNXHashTablePrototype prototype, unsigned capacity, const void *info, void *z)
{
    KSNXHashTable             *table;
    KSNXHashTablePrototype	*proto;
    
    // table = ((KSNXHashTable *)malloc(sizeof (KSNXHashTable)));
    table = ALLOCTABLE(z);
    if (! prototypes) bootstrap();
    // 初始化hash函数表
    if (! prototype.hash)    prototype.hash = KSNXPtrHash;
    if (! prototype.isEqual) prototype.isEqual = KSNXPtrIsEqual;
    if (! prototype.free)    prototype.free = KSNXNoEffectFree;
    if (prototype.style) {
        return NULL;
    };
    proto = (KSNXHashTablePrototype *) KSNXHashGet(prototypes, &prototype);
    if (! proto) {
        proto = (KSNXHashTablePrototype *) malloc(sizeof (KSNXHashTablePrototype));
        bcopy ((const char*)&prototype, (char*)proto, sizeof (KSNXHashTablePrototype));
        (void) KSNXHashInsert (prototypes, proto);
        proto = (KSNXHashTablePrototype *) KSNXHashGet(prototypes, &prototype);
        if (! proto) {
            // KSNXHashInsert插入失败
            return NULL;
        };
    };
    table->prototype = proto;
    table->count = 0;
    table->info = info;
    table->nbBuckets = GOOD_CAPACITY(capacity);
    table->buckets = ALLOCBUCKETS(z, table->nbBuckets);
    
    return table;
}

static void freeBucketPairs (void (*freeProc)(const void *info, void *data), HashBucket bucket, const void *info) {
    unsigned	j = bucket.count;
    const void	**pairs;
    
    if (j == 1) {
        (*freeProc) (info, (void *) bucket.elements.one);
        return;
    };
    pairs = bucket.elements.many;
    while (j--) {
        (*freeProc) (info, (void *) *pairs);
        pairs ++;
    };
    FREEPAIRS (bucket.elements.many);
};

static void freeBuckets (KSNXHashTable *table, int freeObjects) {
    unsigned		i = table->nbBuckets;
    HashBucket		*buckets = (HashBucket *) table->buckets;
    
    while (i--) {
        if (buckets->count) {
            freeBucketPairs ((freeObjects) ? table->prototype->free : KSNXNoEffectFree, *buckets, table->info);
            buckets->count = 0;
            buckets->elements.one = NULL;
        };
        buckets++;
    };
};

void KSNXFreeHashTable (KSNXHashTable *table)
{
    freeBuckets (table, 1);
    free (table->buckets);
    free (table);
};

void KSNXEmptyHashTable (KSNXHashTable *table)
{
    freeBuckets (table, 0);
    table->count = 0;
}

void KSNXResetHashTable (KSNXHashTable *table)
{
    freeBuckets (table, 1);
    table->count = 0;
}

bool KSNXCompareHashTables (KSNXHashTable *table1, KSNXHashTable *table2)
{
    if (table1 == table2) return true;
    if (KSNXCountHashTable (table1) != KSNXCountHashTable (table2)) return false;
    else {
        void		*data;
        KSNXHashState	state = KSNXInitHashState (table1);
        while (KSNXNextHashState (table1, &state, &data)) {
            if (! KSNXHashMember (table2, data)) return false;
        }
        return true;
    }
}

KSNXHashTable *KSNXCopyHashTable (KSNXHashTable *table)
{
    KSNXHashTable		*newt;
    KSNXHashState		state = KSNXInitHashState (table);
    void            *data;
    __unused void	*z = ZONE_FROM_PTR(table);
    
    newt = ALLOCTABLE(z);
    newt->prototype = table->prototype; newt->count = 0;
    newt->info = table->info;
    newt->nbBuckets = table->nbBuckets;
    newt->buckets = ALLOCBUCKETS(z, newt->nbBuckets);
    while (KSNXNextHashState (table, &state, &data))
        KSNXHashInsert (newt, data);
    return newt;
}

unsigned KSNXCountHashTable (KSNXHashTable *table)
{
    return table->count;
}

int KSNXHashMember (KSNXHashTable *table, const void *data)
{
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    
    if (! j) return 0;
    if (j == 1) {
        return ISEQUAL(table, data, bucket->elements.one);
    };
    pairs = bucket->elements.many;
    while (j--) {
        /* we don't cache isEqual because lists are short */
        if (ISEQUAL(table, data, *pairs)) return 1;
        pairs ++;
    };
    return 0;
}

// hash查找函数
/*
 分三种情况
 1，j==0，表明此索引还没有被hash到，直接返回空
 2，j==1，表明只有一个元素，因此使用ISEQUAL与bucket->elements.one进行比较，相同则返回。
 3，j>1，表明多于一个元素，需要进行遍历many数组，逐个比较。
 判断是否两个key是否相同的宏(ISEQUAL)，需要使用上面提到的虚函数表
 */

void *KSNXHashGet(KSNXHashTable *table, const void *data)
{
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    
    if (! j) return NULL;
    if (j == 1) {
        return ISEQUAL(table, data, bucket->elements.one)
        ? (void *) bucket->elements.one : NULL;
    };
    pairs = bucket->elements.many;
    while (j--) {
        /* we don't cache isEqual because lists are short */
        if (ISEQUAL(table, data, *pairs)) return (void *) *pairs;
        pairs ++;
    };
    
    return NULL;
}

void _NXHashRehashToCapacity(KSNXHashTable *table, unsigned newCapacity)
{
    /* Rehash: we create a pseudo table pointing really to the old guys,
     extend self, copy the old pairs, and free the pseudo table */
    KSNXHashTable	*old;
    KSNXHashState	state;
    void	*aux;
    __unused void *z = ZONE_FROM_PTR(table);
    
    old = ALLOCTABLE(z);
    old->prototype = table->prototype;
    old->count = table->count;
    old->nbBuckets = table->nbBuckets;
    old->buckets = table->buckets;
    table->nbBuckets = newCapacity;
    table->count = 0;
    table->buckets = ALLOCBUCKETS(z, table->nbBuckets);
    state = KSNXInitHashState (old);
    while (KSNXNextHashState (old, &state, &aux))
        (void) KSNXHashInsert (table, aux);
    freeBuckets (old, 0);
    if (old->count != table->count)
        printf("*** hashtable: count differs after rehashing; probably indicates a broken invariant: there are x and y such as isEqual(x, y) is TRUE but hash(x) != hash (y)\n");
    free (old->buckets);
    free (old);
}


/*
 当table->count大于Hash数组的个数的时候，开始进行扩容。扩容将新的容量变成原来的容量加1.
 但是注意扩容不能简单的内存copy，这主要是因为BUCKETOF最后的求余是使用的table->nbBuckets。
 也就是说相同的key值在不同的大小的Hash表中具有不同的数组索引，直接内存copy会使原来的Hash失效，
 因此只能采用再次插入的办法，是一个很费效率的工作，因此初始化的时候分配一个差不多适合大小的数组是一个很大的必要。
 */
// #define MORE_CAPACITY(b) (b*2+1)
// 当hash->count > hash->nbBucket时
// 会对hash表进行扩容,扩容后的大小为原来的2倍加1
static void _NXHashRehash(KSNXHashTable *table)
{
    _NXHashRehashToCapacity (table, MORE_CAPACITY(table->nbBuckets));
}

// hash插入函数
/*
 分为三种情况:
 1.j==0 此hash没有元素，直接将data放入bucket->elements.one = data
 2.j==1 已经有一个元素，需要重分配一个两个元素的数组，将data插入前面。newt[1] =bucket->elements.one; *newt = data;
 3.j >1 超过一个元素，同样重分配j+1个元素，然后进行copy，并释放掉原来的内存。
 注意：当table->count大于桶的个数的时候，会进行扩容。
 */
void *KSNXHashInsert (KSNXHashTable *table, const void *data)
{
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    const void	**newt;
    __unused void *z = ZONE_FROM_PTR(table);
    
    if (! j) {
        bucket->count++;
        bucket->elements.one = data;
        table->count++;
        return NULL;
    };
    if (j == 1) {
        if (ISEQUAL(table, data, bucket->elements.one)) {
            const void	*old = bucket->elements.one;
            bucket->elements.one = data;
            return (void *) old;
        };
        newt = ALLOCPAIRS(z, 2);
        newt[1] = bucket->elements.one;
        *newt = data;
        bucket->count++;
        bucket->elements.many = newt;
        table->count++;
        if (table->count > table->nbBuckets) _NXHashRehash (table);
        return NULL;
    };
    pairs = bucket->elements.many;
    while (j--) {
        /* we don't cache isEqual because lists are short */
        if (ISEQUAL(table, data, *pairs)) {
            const void	*old = *pairs;
            *pairs = data;
            return (void *) old;
        };
        pairs ++;
    };
    /* we enlarge this bucket; and put new data in front */
    newt = ALLOCPAIRS(z, bucket->count+1);
    if (bucket->count) bcopy ((const char*)bucket->elements.many, (char*)(newt+1), bucket->count * PTRSIZE);
    *newt = data;
    FREEPAIRS (bucket->elements.many);
    bucket->count++;
    bucket->elements.many = newt;
    table->count++;
    if (table->count > table->nbBuckets) _NXHashRehash (table);
    
    return NULL;
}

void *KSNXHashInsertIfAbsent (KSNXHashTable *table, const void *data)
{
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    const void	**newt;
    __unused void *z = ZONE_FROM_PTR(table);
    
    if (! j) {
        bucket->count++; bucket->elements.one = data;
        table->count++;
        return (void *) data;
    };
    if (j == 1) {
        if (ISEQUAL(table, data, bucket->elements.one))
            return (void *) bucket->elements.one;
        newt = ALLOCPAIRS(z, 2);
        newt[1] = bucket->elements.one;
        *newt = data;
        bucket->count++;
        bucket->elements.many = newt;
        table->count++;
        if (table->count > table->nbBuckets)
            _NXHashRehash (table);
        return (void *) data;
    };
    pairs = bucket->elements.many;
    while (j--) {
        /* we don't cache isEqual because lists are short */
        if (ISEQUAL(table, data, *pairs))
            return (void *) *pairs;
        pairs ++;
    };
    /* we enlarge this bucket; and put new data in front */
    newt = ALLOCPAIRS(z, bucket->count+1);
    if (bucket->count) bcopy ((const char*)bucket->elements.many, (char*)(newt+1), bucket->count * PTRSIZE);
    *newt = data;
    FREEPAIRS (bucket->elements.many);
    bucket->count++;
    bucket->elements.many = newt;
    table->count++;
    if (table->count > table->nbBuckets)
        _NXHashRehash (table);
    
    return (void *) data;
}

void *KSNXHashRemove (KSNXHashTable *table, const void *data)
{
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    const void	**newt;
    __unused void *z = ZONE_FROM_PTR(table);
    
    if (! j) return NULL;
    if (j == 1) {
        if (! ISEQUAL(table, data, bucket->elements.one)) return NULL;
        data = bucket->elements.one;
        table->count--; bucket->count--; bucket->elements.one = NULL;
        return (void *) data;
    };
    pairs = bucket->elements.many;
    if (j == 2) {
        if (ISEQUAL(table, data, pairs[0])) {
            bucket->elements.one = pairs[1]; data = pairs[0];
        }
        else if (ISEQUAL(table, data, pairs[1])) {
            bucket->elements.one = pairs[0]; data = pairs[1];
        }
        else return NULL;
        FREEPAIRS (pairs);
        table->count--; bucket->count--;
        return (void *) data;
    };
    while (j--) {
        if (ISEQUAL(table, data, *pairs)) {
            data = *pairs;
            /* we shrink this bucket */
            newt = (bucket->count-1)
            ? ALLOCPAIRS(z, bucket->count-1) : NULL;
            if (bucket->count-1 != j)
                bcopy ((const char*)bucket->elements.many, (char*)newt, PTRSIZE*(bucket->count-j-1));
            if (j)
                bcopy ((const char*)(bucket->elements.many + bucket->count-j), (char*)(newt+bucket->count-j-1), PTRSIZE*j);
            FREEPAIRS (bucket->elements.many);
            table->count--; bucket->count--; bucket->elements.many = newt;
            return (void *) data;
        };
        pairs ++;
    };
    return NULL;
}

KSNXHashState KSNXInitHashState (KSNXHashTable *table)
{
    KSNXHashState	state;
    
    state.i = table->nbBuckets;
    state.j = 0;
    return state;
};

int KSNXNextHashState (KSNXHashTable *table, KSNXHashState *state, void **data)
{
    HashBucket		*buckets = (HashBucket *) table->buckets;
    
    while (state->j == 0) {
        if (state->i == 0) return 0;
        state->i--; state->j = buckets[state->i].count;
    }
    state->j--;
    buckets += state->i;
    *data = (void *) ((buckets->count == 1)
                      ? buckets->elements.one : buckets->elements.many[state->j]);
    return 1;
};

/*************************************************************************
 *
 *	Conveniences
 *  初始化hash函数表(prototype)的函数
 *	针对ptr和str采用不同的函数
 *
 *************************************************************************/

uintptr_t KSNXPtrHash (const void *info, const void *data)
{
    return (((uintptr_t) data) >> 16) ^ ((uintptr_t) data);
};

uintptr_t KSNXStrHash (const void *info, const void *data)
{
    uintptr_t hash = 0;
    unsigned char *s = (unsigned char *) data;
    /* unsigned to avoid a sign-extend */
    /* unroll the loop */
    if (s) for (; ; ) {
        if (*s == '\0') break;
        hash ^= (uintptr_t) *s++;
        if (*s == '\0') break;
        hash ^= (uintptr_t) *s++ << 8;
        if (*s == '\0') break;
        hash ^= (uintptr_t) *s++ << 16;
        if (*s == '\0') break;
        hash ^= (uintptr_t) *s++ << 24;
    }
    return hash;
};

int KSNXStrIsEqual (const void *info, const void *data1, const void *data2)
{
    if (data1 == data2) return 1;
    if (! data1) return ! strlen ((char *) data2);
    if (! data2) return ! strlen ((char *) data1);
    if (((char *) data1)[0] != ((char *) data2)[0]) return 0;
    return (strcmp ((char *) data1, (char *) data2)) ? 0 : 1;
};

void KSNXReallyFree (const void *info, void *data)
{
    free (data);
};

/* All the following functions are really private, made non-static only for the benefit of shlibs */
static uintptr_t hashPtrStructKey (const void *info, const void *data)
{
    return KSNXPtrHash(info, *((void **) data));
};

static int isEqualPtrStructKey (const void *info, const void *data1, const void *data2)
{
    return KSNXPtrIsEqual (info, *((void **) data1), *((void **) data2));
};

static uintptr_t hashStrStructKey (const void *info, const void *data)
{
    return KSNXStrHash(info, *((char **) data));
};

static int isEqualStrStructKey (const void *info, const void *data1, const void *data2)
{
    return KSNXStrIsEqual (info, *((char **) data1), *((char **) data2));
};

const KSNXHashTablePrototype KSNXPtrPrototype = {
    KSNXPtrHash, KSNXPtrIsEqual, KSNXNoEffectFree, 0
};

const KSNXHashTablePrototype KSNXStrPrototype = {
    KSNXStrHash, KSNXStrIsEqual, KSNXNoEffectFree, 0
};

const KSNXHashTablePrototype KSNXPtrStructKeyPrototype = {
    hashPtrStructKey, isEqualPtrStructKey, KSNXReallyFree, 0
};

const KSNXHashTablePrototype KSNXStrStructKeyPrototype = {
    hashStrStructKey, isEqualStrStructKey, KSNXReallyFree, 0
};

/*************************************************************************
 *
 *	Unique strings
 *
 *************************************************************************/

#if !__OBJC2__  &&  !TARGET_OS_WIN32
/* the implementation could be made faster at the expense of memory if the size of the strings were kept around */
static KSNXHashTable *uniqueStrings = NULL;

/* this is based on most apps using a few K of strings, and an average string size of 15 using sqrt(2*dataAlloced*perChunkOverhead) */
#define CHUNK_SIZE	360

static int accessUniqueString = 0;

static char		*z = NULL;
static size_t	zSize = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static const char *CopyIntoReadOnly (const char *str)
{
    size_t	len = strlen (str) + 1;
    char	*result;
    
    if (len > CHUNK_SIZE/2) {	/* dont let big strings waste space */
        result = (char *)malloc (len);
        bcopy (str, result, len);
        return result;
    }
    
    pthread_mutex_lock(&lock);
    if (zSize < len) {
        zSize = CHUNK_SIZE *((len + CHUNK_SIZE - 1) / CHUNK_SIZE);
        /* not enough room, we try to allocate.  If no room left, too bad */
        z = (char *)malloc (zSize);
    };
    
    result = z;
    bcopy (str, result, len);
    z += len;
    zSize -= len;
    pthread_mutex_unlock(&lock);
    return result;
};

NXAtom NXUniqueString (const char *buffer)
{
    const char	*previous;
    
    if (! buffer) return buffer;
    accessUniqueString++;
    if (! uniqueStrings)
        uniqueStrings = KSNXCreateHashTable (KSNXStrPrototype, 0, NULL);
    previous = (const char *) KSNXHashGet (uniqueStrings, buffer);
    if (previous) return previous;
    previous = CopyIntoReadOnly (buffer);
    if (KSNXHashInsert (uniqueStrings, previous)) {
        printf("*** NXUniqueString: invariant broken\n");
        return NULL;
    };
    return previous;
};

NXAtom NXUniqueStringNoCopy (const char *string)
{
    accessUniqueString++;
    if (! uniqueStrings)
        uniqueStrings = KSNXCreateHashTable (KSNXStrPrototype, 0, NULL);
    return (const char *) KSNXHashInsertIfAbsent (uniqueStrings, string);
};

#define BUF_SIZE	256

NXAtom NXUniqueStringWithLength (const char *buffer, int length)
{
    NXAtom	atom;
    char	*nullTermStr;
    char	stackBuf[BUF_SIZE];
    
    if (length+1 > BUF_SIZE)
        nullTermStr = (char *)malloc (length+1);
    else
        nullTermStr = stackBuf;
    bcopy (buffer, nullTermStr, length);
    nullTermStr[length] = '\0';
    atom = NXUniqueString (nullTermStr);
    if (length+1 > BUF_SIZE)
        free (nullTermStr);
    return atom;
};

char *NXCopyStringBufferFromZone (const char *str, void *zone)
{
#if !SUPPORT_ZONES
    return strdup(str);
#else
    return strcpy ((char *) malloc_zone_malloc((malloc_zone_t *)zone, strlen (str) + 1), str);
#endif
};

char *NXCopyStringBuffer (const char *str)
{
    return strdup(str);
};

#endif
