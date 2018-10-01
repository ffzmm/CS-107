/*
 * File: cvector.c
 * Author: Iris Yang 
 * ----------------------
 *
 */

#include "cvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <search.h>

// a suggested value to use when given capacity_hint is 0
#define DEFAULT_CAPACITY 16

/* Type: struct CVectorImplementation
 * ----------------------------------
 * This definition completes the CVector type that was declared in
 * cvector.h. You fill in the struct with your chosen fields.
 */
struct CVectorImplementation {
  void * elems;
  size_t  elemsz, capacity;
  int nelems;
  CleanupElemFn fn;
};


/* The NOT_YET_IMPLEMENTED macro is used as the body for all functions
 * to remind you about which operations you haven't yet implemented.
 * It will report a fatal error if a call is made to an not-yet-implemented
 * function (this is preferable to returning garbage or silently
 * ignoring the call).  Remove the call to this macro as you implement
 * each function and finally you can remove this macro and comment
 * when no longer needed.
 */
#define NOT_YET_IMPLEMENTED printf("%s() not yet implemented!\n", __func__); raise(SIGKILL); exit(107);

/* Helper function to access nth element */
static void *get_nth(const CVector *cv, int index)
{
  return (void*)( (char*)(cv->elems) + index*cv->elemsz );
}

/* Helper to dynamically allocate array */
static void checkCapacity(CVector *cv)
{
  if (1.0*cv->capacity/cv->nelems < 2) {
      cv->elems = realloc(cv->elems, 2*(cv->elemsz)*(cv->capacity));
      assert(cv->elems); // assert successful allocation
      cv->capacity *= 2;
  }
} 

/* Helper for checking index within [0, count-1] */
static void assert_bound(int index, const CVector *cv)
{
  assert(index >= 0);
  assert(index < cv->nelems);
}

CVector *cvec_create(size_t elemsz, size_t capacity_hint, CleanupElemFn fn)
{
  assert(elemsz > 0);
  CVector* new = malloc(sizeof(struct CVectorImplementation));
  assert(new);
  new->elemsz = elemsz;
  new->capacity = (capacity_hint > 0) ? capacity_hint : DEFAULT_CAPACITY;
  new->elems = malloc(elemsz*new->capacity);
  new->nelems = 0;
  new->fn = fn;
  return new;
}
 

void cvec_dispose(CVector *cv)
{
  for(int i = 0; i < (cv->nelems); i++) {
    if(cv->fn) (cv->fn)(get_nth(cv, i));
  }
  free(cv->elems);
  free(cv);
}

int cvec_count(const CVector *cv)
{
  return cv->nelems;
}

void *cvec_nth(const CVector *cv, int index)
{
  assert_bound(index, cv);
  return get_nth(cv, index);
}

void cvec_insert(CVector *cv, const void *addr, int index)
{
  assert(addr);
  if(index == cv->nelems)
    cvec_append(cv, addr);
  else {
    checkCapacity(cv);
    assert_bound(index, cv);
    memmove(get_nth(cv, index+1), get_nth(cv, index), cv->elemsz*(cv->nelems-index));
    memcpy(get_nth(cv, index), addr, cv->elemsz);
    cv->nelems++;
  }
}
 
void cvec_append(CVector *cv, const void *addr)
{
  checkCapacity(cv);
  memcpy(get_nth(cv, cv->nelems), addr, cv->elemsz);
  cv->nelems++;
}


void cvec_replace(CVector *cv, const void *addr, int index)
{
  assert_bound(index, cv);
  void *toRemove = get_nth(cv, index);
  if (cv->fn) (cv->fn)(toRemove);
  memcpy(toRemove, addr, cv->elemsz);
}

void cvec_remove(CVector *cv, int index)
{
  assert_bound(index, cv);
  void *toRemove = get_nth(cv, index);
  if (cv->fn) (cv->fn)(toRemove);
  memmove(get_nth(cv, index), get_nth(cv, index+1), cv->elemsz*(cv->nelems-index-1));
  cv->nelems--;
}

int cvec_search(const CVector *cv, const void *key, CompareFn cmp, int start, bool sorted)
{
  assert(start >= 0);
  assert(start <= cv->nelems);

  void *ptr = NULL;
  size_t NtoSearch = cv->nelems - start;
  if(sorted) 
    ptr = bsearch(key, get_nth(cv, start), NtoSearch, cv->elemsz, cmp);
  else 
    ptr = lfind(key, get_nth(cv, start), &NtoSearch, cv->elemsz, cmp);

  if(!ptr) return -1;
  else return ((char*)ptr - (char*)cv->elems)/cv->elemsz;

}

void cvec_sort(CVector *cv, CompareFn cmp)
{
  qsort(cv->elems, cv->nelems, cv->elemsz, cmp);
}

void *cvec_first(const CVector *cv)
{
  if (cv->nelems == 0) return NULL;
  return get_nth(cv, 0);
}

void *cvec_next(const CVector *cv, const void *prev)
{
  if (cv->nelems == 0) return NULL;
  char* end = get_nth(cv, cv->nelems-1);
  if ((char*) prev + cv->elemsz <= end)
    return (void*)((char*)prev + (cv->elemsz));
  else // out of bound
    return NULL;
}
