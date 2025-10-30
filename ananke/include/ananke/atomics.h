/++
    Module Name:

        atomics.h

    Abstract:

        Atomic operations, interlocked functions, and reference counting.

    Environment:

        C and C++ compatible.
--/

#pragma once

#include <ananke/compiler.h>
#include <ananke/types.h>

/* --------------------------------------------------------------- */
/*  Atomics & refcount (portable functions + macro front-ends).    */
/* --------------------------------------------------------------- */
static INLINE INT32 AnxInterlockedIncrement(volatile INT32* p);
static INLINE INT32 AnxInterlockedDecrement(volatile INT32* p);
static INLINE INT32 AnxInterlockedExchange (volatile INT32* p, INT32 v);
static INLINE INT32 AnxInterlockedCompareAndExchange(volatile INT32* p, INT32 nv, INT32 ov);
static INLINE INT32 AnxInterlockedAdd      (volatile INT32* p, INT32 v);
static INLINE INT32 AnxInterlockedOr       (volatile INT32* p, INT32 v);
static INLINE INT32 AnxInterlockedAnd      (volatile INT32* p, INT32 v);
static INLINE INT32 AnxInterlockedXor      (volatile INT32* p, INT32 v);
/* 64-bit primitives */
static INLINE INT64 AnxInterlockedIncrement64(volatile INT64* p);
static INLINE INT64 AnxInterlockedDecrement64(volatile INT64* p);
static INLINE INT64 AnxInterlockedExchange64 (volatile INT64* p, INT64 v);
static INLINE INT64 AnxInterlockedAdd64      (volatile INT64* p, INT64 v);
static INLINE INT64 AnxInterlockedOr64       (volatile INT64* p, INT64 v);
static INLINE INT64 AnxInterlockedAnd64      (volatile INT64* p, INT64 v);
static INLINE INT64 AnxInterlockedXor64      (volatile INT64* p, INT64 v);
static INLINE INT64 AnxInterlockedCompareAndExchange64(volatile INT64* p, INT64 nv, INT64 ov);

#if defined(_MSC_VER)
#   include <intrin.h>
#   pragma intrinsic(_InterlockedIncrement)
#   pragma intrinsic(_InterlockedDecrement)
#   pragma intrinsic(_InterlockedExchange)
#   pragma intrinsic(_InterlockedCompareExchange)
#   pragma intrinsic(_InterlockedExchangeAdd)
#   pragma intrinsic(_InterlockedOr)
#   pragma intrinsic(_InterlockedAnd)
#   pragma intrinsic(_InterlockedXor)
#   if defined(_M_X64)
#       pragma intrinsic(_InterlockedExchange64)
#       pragma intrinsic(_InterlockedExchangeAdd64)
#       pragma intrinsic(_InterlockedOr64)
#       pragma intrinsic(_InterlockedAnd64)
#       pragma intrinsic(_InterlockedXor64)
#       pragma intrinsic(_InterlockedCompareExchange64)
#   endif
    /* 32-bit */
    static INLINE INT32 AnxInterlockedIncrement(volatile INT32* p){ return _InterlockedIncrement((long volatile*)p); }
    static INLINE INT32 AnxInterlockedDecrement(volatile INT32* p){ return _InterlockedDecrement((long volatile*)p); }
    static INLINE INT32 AnxInterlockedExchange (volatile INT32* p, INT32 v){ return _InterlockedExchange((long volatile*)p,(long)v); }
    static INLINE INT32 AnxInterlockedCompareAndExchange(volatile INT32* p, INT32 nv, INT32 ov){ return _InterlockedCompareExchange((long volatile*)p,(long)nv,(long)ov); }
    static INLINE INT32 AnxInterlockedAdd      (volatile INT32* p, INT32 v){ return _InterlockedExchangeAdd((long volatile*)p,(long)v) + v; }
    static INLINE INT32 AnxInterlockedOr       (volatile INT32* p, INT32 v){ return _InterlockedOr((long volatile*)p,(long)v); }
    static INLINE INT32 AnxInterlockedAnd      (volatile INT32* p, INT32 v){ return _InterlockedAnd((long volatile*)p,(long)v); }
    static INLINE INT32 AnxInterlockedXor      (volatile INT32* p, INT32 v){ return _InterlockedXor((long volatile*)p,(long)v); }
    /* 64-bit */
#   if defined(_M_X64)
    static INLINE INT64 AnxInterlockedIncrement64(volatile INT64* p){ return _InterlockedExchangeAdd64((long long volatile*)p, 1) + 1; }
    static INLINE INT64 AnxInterlockedDecrement64(volatile INT64* p){ return _InterlockedExchangeAdd64((long long volatile*)p,-1) - 1; }
    static INLINE INT64 AnxInterlockedExchange64 (volatile INT64* p, INT64 v){ return _InterlockedExchange64((long long volatile*)p,(long long)v); }
    static INLINE INT64 AnxInterlockedCompareAndExchange64(volatile INT64* p, INT64 nv, INT64 ov){ return _InterlockedCompareExchange64((long long volatile*)p,(long long)nv,(long long)ov); }
    static INLINE INT64 AnxInterlockedAdd64      (volatile INT64* p, INT64 v){ return _InterlockedExchangeAdd64((long long volatile*)p,(long long)v) + v; }
    static INLINE INT64 AnxInterlockedOr64       (volatile INT64* p, INT64 v){ return _InterlockedOr64((long long volatile*)p,(long long)v); }
    static INLINE INT64 AnxInterlockedAnd64      (volatile INT64* p, INT64 v){ return _InterlockedAnd64((long long volatile*)p,(long long)v); }
    static INLINE INT64 AnxInterlockedXor64      (volatile INT64* p, INT64 v){ return _InterlockedXor64((long long volatile*)p,(long long)v); }
#   else
    static INLINE INT64 AnxInterlockedIncrement64(volatile INT64* p){ INT64 v=*p; v++; *p=v; return v; }
    static INLINE INT64 AnxInterlockedDecrement64(volatile INT64* p){ INT64 v=*p; v--; *p=v; return v; }
    static INLINE INT64 AnxInterlockedExchange64 (volatile INT64* p, INT64 v){ INT64 o=*p; *p=v; return o; }
    static INLINE INT64 AnxInterlockedCompareAndExchange64(volatile INT64* p, INT64 nv, INT64 ov){ INT64 o=*p; if(o==ov)*p=nv; return o; }
    static INLINE INT64 AnxInterlockedAdd64      (volatile INT64* p, INT64 v){ INT64 o=*p; *p=o+v; return o+v; }
    static INLINE INT64 AnxInterlockedOr64       (volatile INT64* p, INT64 v){ INT64 o=*p; *p=o|v; return o; }
    static INLINE INT64 AnxInterlockedAnd64      (volatile INT64* p, INT64 v){ INT64 o=*p; *p=o&v; return o; }
    static INLINE INT64 AnxInterlockedXor64      (volatile INT64* p, INT64 v){ INT64 o=*p; *p=o^v; return o; }
#   endif

#elif defined(__clang__) || defined(__GNUC__)
    /* Prefer __atomic */
#   if defined(__ATOMIC_SEQ_CST)
    static INLINE INT32 AnxInterlockedIncrement(volatile INT32* p){ return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST); }
    static INLINE INT32 AnxInterlockedDecrement(volatile INT32* p){ return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST); }
    static INLINE INT32 AnxInterlockedExchange (volatile INT32* p, INT32 v){ INT32 o; __atomic_exchange(p, &v, &o, __ATOMIC_SEQ_CST); return o; }
    static INLINE INT32 AnxInterlockedCompareAndExchange(volatile INT32* p, INT32 nv, INT32 ov){ __atomic_compare_exchange(p, &ov, &nv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return ov; }
    static INLINE INT32 AnxInterlockedAdd      (volatile INT32* p, INT32 v){ return __atomic_add_fetch(p, v, __ATOMIC_SEQ_CST); }
    static INLINE INT32 AnxInterlockedOr       (volatile INT32* p, INT32 v){ return __atomic_fetch_or(p, v, __ATOMIC_SEQ_CST); }
    static INLINE INT32 AnxInterlockedAnd      (volatile INT32* p, INT32 v){ return __atomic_fetch_and(p, v, __ATOMIC_SEQ_CST); }
    static INLINE INT32 AnxInterlockedXor      (volatile INT32* p, INT32 v){ return __atomic_fetch_xor(p, v, __ATOMIC_SEQ_CST); }

    static INLINE INT64 AnxInterlockedIncrement64(volatile INT64* p){ return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST); }
    static INLINE INT64 AnxInterlockedDecrement64(volatile INT64* p){ return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST); }
    static INLINE INT64 AnxInterlockedExchange64 (volatile INT64* p, INT64 v){ INT64 o; __atomic_exchange(p, &v, &o, __ATOMIC_SEQ_CST); return o; }
    static INLINE INT64 AnxInterlockedCompareAndExchange64(volatile INT64* p, INT64 nv, INT64 ov){ __atomic_compare_exchange(p, &ov, &nv, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); return ov; }
    static INLINE INT64 AnxInterlockedAdd64      (volatile INT64* p, INT64 v){ return __atomic_add_fetch(p, v, __ATOMIC_SEQ_CST); }
    static INLINE INT64 AnxInterlockedOr64       (volatile INT64* p, INT64 v){ return __atomic_fetch_or(p, v, __ATOMIC_SEQ_CST); }
    static INLINE INT64 AnxInterlockedAnd64      (volatile INT64* p, INT64 v){ return __atomic_fetch_and(p, v, __ATOMIC_SEQ_CST); }
    static INLINE INT64 AnxInterlockedXor64      (volatile INT64* p, INT64 v){ return __atomic_fetch_xor(p, v, __ATOMIC_SEQ_CST); }
#   else
    static INLINE INT32 AnxInterlockedIncrement(volatile INT32* p){ return __sync_add_and_fetch(p, 1); }
    static INLINE INT32 AnxInterlockedDecrement(volatile INT32* p){ return __sync_sub_and_fetch(p, 1); }
    static INLINE INT32 AnxInterlockedExchange (volatile INT32* p, INT32 v){ INT32 o; do { o = *p; } while(!__sync_bool_compare_and_swap(p, o, v)); return o; }
    static INLINE INT32 AnxInterlockedCompareAndExchange(volatile INT32* p, INT32 nv, INT32 ov){ return __sync_val_compare_and_swap(p, ov, nv); }
    static INLINE INT32 AnxInterlockedAdd      (volatile INT32* p, INT32 v){ return __sync_add_and_fetch(p, v); }
    static INLINE INT32 AnxInterlockedOr       (volatile INT32* p, INT32 v){ return __sync_fetch_and_or(p, v); }
    static INLINE INT32 AnxInterlockedAnd      (volatile INT32* p, INT32 v){ return __sync_fetch_and_and(p, v); }
    static INLINE INT32 AnxInterlockedXor      (volatile INT32* p, INT32 v){ return __sync_fetch_and_xor(p, v); }

    static INLINE INT64 AnxInterlockedIncrement64(volatile INT64* p){ return __sync_add_and_fetch(p, 1); }
    static INLINE INT64 AnxInterlockedDecrement64(volatile INT64* p){ return __sync_sub_and_fetch(p, 1); }
    static INLINE INT64 AnxInterlockedExchange64 (volatile INT64* p, INT64 v){ INT64 o; do { o = *p; } while(!__sync_bool_compare_and_swap(p, o, v)); return o; }
    static INLINE INT64 AnxInterlockedCompareAndExchange64(volatile INT64* p, INT64 nv, INT64 ov){ return __sync_val_compare_and_swap(p, ov, nv); }
    static INLINE INT64 AnxInterlockedAdd64      (volatile INT64* p, INT64 v){ return __sync_add_and_fetch(p, v); }
    static INLINE INT64 AnxInterlockedOr64       (volatile INT64* p, INT64 v){ return __sync_fetch_and_or(p, v); }
    static INLINE INT64 AnxInterlockedAnd64      (volatile INT64* p, INT64 v){ return __sync_fetch_and_and(p, v); }
    static INLINE INT64 AnxInterlockedXor64      (volatile INT64* p, INT64 v){ return __sync_fetch_and_xor(p, v); }
#   endif

#else /* Watcom / generic C */
    static INLINE INT32 AnxInterlockedIncrement(volatile INT32* p){ INT32 v=*p; v++; *p=v; return v; }
    static INLINE INT32 AnxInterlockedDecrement(volatile INT32* p){ INT32 v=*p; v--; *p=v; return v; }
    static INLINE INT32 AnxInterlockedExchange (volatile INT32* p, INT32 v){ INT32 o=*p; *p=v; return o; }
    static INLINE INT32 AnxInterlockedCompareAndExchange(volatile INT32* p, INT32 nv, INT32 ov){ INT32 o=*p; if(o==ov)*p=nv; return o; }
    static INLINE INT32 AnxInterlockedAdd      (volatile INT32* p, INT32 v){ INT32 o=*p; *p=o+v; return o+v; }
    static INLINE INT32 AnxInterlockedOr       (volatile INT32* p, INT32 v){ INT32 o=*p; *p=o|v; return o; }
    static INLINE INT32 AnxInterlockedAnd      (volatile INT32* p, INT32 v){ INT32 o=*p; *p=o&v; return o; }
    static INLINE INT32 AnxInterlockedXor      (volatile INT32* p, INT32 v){ INT32 o=*p; *p=o^v; return o; }

    static INLINE INT64 AnxInterlockedIncrement64(volatile INT64* p){ INT64 v=*p; v++; *p=v; return v; }
    static INLINE INT64 AnxInterlockedDecrement64(volatile INT64* p){ INT64 v=*p; v--; *p=v; return v; }
    static INLINE INT64 AnxInterlockedExchange64 (volatile INT64* p, INT64 v){ INT64 o=*p; *p=v; return o; }
    static INLINE INT64 AnxInterlockedCompareAndExchange64(volatile INT64* p, INT64 nv, INT64 ov){ INT64 o=*p; if(o==ov)*p=nv; return o; }
    static INLINE INT64 AnxInterlockedAdd64      (volatile INT64* p, INT64 v){ INT64 o=*p; *p=o+v; return o+v; }
    static INLINE INT64 AnxInterlockedOr64       (volatile INT64* p, INT64 v){ INT64 o=*p; *p=o|v; return o; }
    static INLINE INT64 AnxInterlockedAnd64      (volatile INT64* p, INT64 v){ INT64 o=*p; *p=o&v; return o; }
    static INLINE INT64 AnxInterlockedXor64      (volatile INT64* p, INT64 v){ INT64 o=*p; *p=o^v; return o; }
#endif

#define ANX_INTERLOCKED_INCREMENT(p)            AnxInterlockedIncrement(p)
#define ANX_INTERLOCKED_DECREMENT(p)            AnxInterlockedDecrement(p)
#define ANX_INTERLOCKED_EXCHANGE(p,v)           AnxInterlockedExchange((p),(v))
#define ANX_INTERLOCKED_CMPXCHG(p,nv,ov)        AnxInterlockedCompareAndExchange((p),(nv),(ov))
#define ANX_INTERLOCKED_ADD(p,v)                AnxInterlockedAdd((p),(v))
#define ANX_INTERLOCKED_OR(p,v)                 AnxInterlockedOr((p),(v))
#define ANX_INTERLOCKED_AND(p,v)                AnxInterlockedAnd((p),(v))
#define ANX_INTERLOCKED_XOR(p,v)                AnxInterlockedXor((p),(v))
#define ANX_INTERLOCKED_INCREMENT64(p)          AnxInterlockedIncrement64(p)
#define ANX_INTERLOCKED_DECREMENT64(p)          AnxInterlockedDecrement64(p)
#define ANX_INTERLOCKED_EXCHANGE64(p,v)         AnxInterlockedExchange64((p),(v))
#define ANX_INTERLOCKED_ADD64(p,v)              AnxInterlockedAdd64((p),(v))
#define ANX_INTERLOCKED_OR64(p,v)               AnxInterlockedOr64((p),(v))
#define ANX_INTERLOCKED_AND64(p,v)              AnxInterlockedAnd64((p),(v))
#define ANX_INTERLOCKED_XOR64(p,v)              AnxInterlockedXor64((p),(v))
#define ANX_INTERLOCKED_CMPXCHG64(p,nv,ov)      AnxInterlockedCompareAndExchange64((p),(nv),(ov))

typedef struct _REFOBJ { VOLATILE INT32 RefCount; } REFOBJ;
#define ANX_REF_INC(o)  ((UINT32)ANX_INTERLOCKED_INCREMENT(&(o)->RefCount))
#define ANX_REF_DEC(o)  ((UINT32)ANX_INTERLOCKED_DECREMENT(&(o)->RefCount))

/* Ensure 64-bit MSVC intrinsics are only used when truly 64-bit. */
#if defined(_MSC_VER) && !defined(_M_X64)
#   undef ANX_INTERLOCKED_INCREMENT64
#   undef ANX_INTERLOCKED_DECREMENT64
#   undef ANX_INTERLOCKED_EXCHANGE64
#   undef ANX_INTERLOCKED_ADD64
#   undef ANX_INTERLOCKED_OR64
#   undef ANX_INTERLOCKED_AND64
#   undef ANX_INTERLOCKED_XOR64
#   undef ANX_INTERLOCKED_CMPXCHG64
    /* Re-map to safe inline fallbacks already defined above. */
#   define ANX_INTERLOCKED_INCREMENT64(p)          AnxInterlockedIncrement64(p)
#   define ANX_INTERLOCKED_DECREMENT64(p)          AnxInterlockedDecrement64(p)
#   define ANX_INTERLOCKED_EXCHANGE64(p,v)         AnxInterlockedExchange64((p),(v))
#   define ANX_INTERLOCKED_ADD64(p,v)              AnxInterlockedAdd64((p),(v))
#   define ANX_INTERLOCKED_OR64(p,v)               AnxInterlockedOr64((p),(v))
#   define ANX_INTERLOCKED_AND64(p,v)              AnxInterlockedAnd64((p),(v))
#   define ANX_INTERLOCKED_XOR64(p,v)              AnxInterlockedXor64((p),(v))
#   define ANX_INTERLOCKED_CMPXCHG64(p,nv,ov)      AnxInterlockedCompareAndExchange64((p),(nv),(ov))
#endif
