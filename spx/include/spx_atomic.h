#ifndef _SPX_ATOMIC_H_
#define _SPX_ATOMIC_H_
#ifdef __cplusplus
extern "C" {
#endif

#define SpxAtomicAdd(ptr,v) __sync_add_and_fetch(ptr,v)
#define SpxAtomicSub(ptr,v) __sync_sub_and_fetch(ptr,v)
#define SpxAtomicIncr(ptr) __sync_add_and_fetch(ptr,1)
#define SpxAtomicDecr(ptr) __sync_sub_and_fetch(ptr,1)
#define SpxAtomicLazyAdd(ptr,v) __sync_fetch_and_add(ptr,v)
#define SpxAtomicLazySub(ptr,v) __sync_fetch_and_sub(ptr,v)
#define SpxAtomicLazyIncr(ptr) __sync_fetch_and_add(ptr,1)
#define SpxAtomicLazyDecr(ptr) __sync_fetch_and_sub(ptr,1)
#define SpxAtomicRelease(ptr) __sync_lock_release(ptr)
#define SpxAtomicSet(ptr,v) __sync_lock_test_and_set(ptr,v)
#define SpxAtomicIsCas(ptr,o,v) __sync_bool_compare_and_swap(ptr,o,v)
#define SpxAtomicCas(ptr,o,v) __sync_val_compare_and_swap(ptr,o,v)



#define SpxAtomicVAdd(v,a) __sync_add_and_fetch(&(v),a)
#define SpxAtomicVSub(v,s) __sync_sub_and_fetch(&(v),s)
#define SpxAtomicVIncr(v) __sync_add_and_fetch(&(v),1)
#define SpxAtomicVDecr(v) __sync_sub_and_fetch(&(v),1)
#define SpxAtomicLazyVAdd(v,a) __sync_fetch_and_add(&(v),a)
#define SpxAtomicLazyVSub(v,s) __sync_fetch_and_sub(&(v),s)
#define SpxAtomicLazyVIncr(v) __sync_fetch_and_add(&(v),1)
#define SpxAtomicLazyVDecr(v) __sync_fetch_and_sub(&(v),1)
#define SpxAtomicVRelease(v) __sync_lock_release(&(v))
#define SpxAtomicVSet(v,n) __sync_lock_test_and_set(&(v),n)
#define SpxAtomicVIsCas(v,o,n) __sync_bool_compare_and_swap(&(v),o,n)
#define SpxAtomicVCas(v,o,n) __sync_val_compare_and_swap(&(v),o,n)



#define SpxAtomicMB() __sync_synchronize()
#define SpxAtomicRMB() SpxAtomicMB()
#define SpxAtomicWMB() SpxAtomicMB()


#ifdef __cplusplus
}
#endif
#endif
