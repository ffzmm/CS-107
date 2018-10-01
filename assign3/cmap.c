/*
 * File: cmap.c
 * Author: Iris Yang 
 * ----------------------
 *
 */

#include "cmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>

// a suggested value to use when given capacity_hint is 0
#define DEFAULT_CAPACITY 1023

/* Type: struct CMapImplementation
 * -------------------------------
 * This definition completes the CMap type that was declared in
 * cmap.h. You fill in the struct with your chosen fields.
 */
struct CMapImplementation {
  size_t valuesz, nelems;
  int nbuckets;
  void** buckets;
  CleanupValueFn fn;
};



/* The NOT_YET_IMPLEMENTED macro is used as the body for all functions
 * to remind you about which operations you haven't yet implemented.
 * It wil report an error if a call is made to an not-yet-implemented
 * function (this is preferable to returning garbage or silently
 * ignoring the call).  Remove the call to this macro as you implement
 * each function and finally you can remove this macro and comment
 * when no longer needed.
 */
#define NOT_YET_IMPLEMENTED printf("%s() not yet implemented!\n", __func__); raise(SIGKILL); exit(107);



/* Function: hash
 * --------------
 * This function adapted from Eric Roberts' _The Art and Science of C_
 * It takes a string and uses it to derive a "hash code," which
 * is an integer in the range [0..nbuckets-1]. The hash code is computed
 * using a method called "linear congruence." A similar function using this
 * method is described on page 144 of Kernighan and Ritchie. The choice of
 * the value for the multiplier can have a significant effort on the
 * performance of the algorithm, but not on its correctness.
 * The computed hash value is stable, e.g. passing the same string and
 * nbuckets to function again will always return the same code.
 * The hash is case-sensitive, "ZELENSKI" and "Zelenski" are
 * not guaranteed to hash to same code.
 */
static int hash(const char *s, int nbuckets)
{
   const unsigned long MULTIPLIER = 2630849305L; // magic number
   unsigned long hashcode = 0;
   for (int i = 0; s[i] != '\0'; i++)
      hashcode = hashcode * MULTIPLIER + s[i];
   return hashcode % nbuckets;
}


/* Helper to return the pointer to the next connected blob */
static void *getNextBlob(const void *blob)
{
  return *(void**)blob; 
}

/* Helper to get the key of cell "blob" */
static char *getBlobKey(const void* blob)
{
  return (char*) blob + sizeof(void*);
}

/* Helper to get the pointer to the value in the blob */
static void *getBlobValPtr(const void* blob)
{
  char *key = getBlobKey(blob);
  return (void*)(key + strlen(key) + 1);
}

/* Helper to set three fields in blob */
static void setBlob(const CMap *cm, void * blob, void* nextPtr, const char *key, const void *valAddr)
{
  //*(void**)blob = nextPtr;
  memcpy(blob, nextPtr, sizeof(char*));
  memcpy((char*)blob + sizeof(void*), key, strlen(key)+1);
  memcpy(getBlobValPtr(blob), valAddr, cm->valuesz);
}

/* Helper to return the ptr to the last blob in a linked list */
static void *findEnd(void *list)
{
  void *cur = list;
  while(getNextBlob(cur))  cur = getNextBlob(cur);
  return cur;
}

/* Helper to return the ptr to blob containing a certain key */
static void *getBlobFromKey(const CMap *cm, const char *key)
{
  int buk = hash(key, cm->nbuckets);
  for (void *cur = (cm->buckets)[buk]; cur != NULL; cur = getNextBlob(cur)){
    if( strcmp(key, getBlobKey(cur)) == 0 ) 
      return cur; 
  }
  return NULL; // no corresponding blob found   
}

/* Helper to clean linked list in bucket i */
static void cleanBucket_i(CMap *cm, void *list)
{
  void *cur = list; // list != NULL
  while(cur){
    void *next = getNextBlob(cur);
    if(cm->fn) (cm->fn)(getBlobValPtr(cur));
    free(cur);
    cur = next;
  }
}

/* Helper to determine if the blob has been "removed" */
static int isEmptyBlob(const void* blob)
{
  return (getBlobKey(blob)[0] == '\0');
}

CMap *cmap_create(size_t valuesz, size_t capacity_hint, CleanupValueFn fn)
{
  CMap *new = malloc(sizeof(struct CMapImplementation));
  assert(new);
  assert(valuesz > 0);
  new->valuesz = valuesz;
  new->nbuckets = (capacity_hint > 0) ? capacity_hint : DEFAULT_CAPACITY;
  new->buckets = malloc((new->nbuckets)*sizeof(void*));
  assert(new->buckets);
  for(int i = 0; i < new->nbuckets; i++) 
    (new->buckets)[i] = NULL;
  new->fn = fn;
  new->nelems = 0;

  return new;  
}

int cmap_count(const CMap *cm)
{
  return cm->nelems;
}

void cmap_put(CMap *cm, const char *key, const void *addr)
{
  if ( !cmap_get(cm, key) ) { // no key present in the map
    int idx = hash(key, cm->nbuckets);

    void **endBlob; // declares a ptr to (void*) where (void*) is NULL (end of the list) 
    if( !(cm->buckets)[idx] ) endBlob = cm->buckets+idx;
    else
      endBlob = findEnd( (cm->buckets)[idx] );

    void *newBlob = malloc(2*sizeof(void*) + strlen(key)+1 + sizeof(cm->valuesz));
    assert(newBlob);
    *endBlob = newBlob; // link newBlob with endBlob
    setBlob(cm, newBlob, NULL, key, addr);
    (cm->nelems)++;  

  } else { // key already in the map

    void *toOverWritten = cmap_get(cm, key);
    if(cm->fn) (cm->fn)(toOverWritten); // call cleanup function
    memcpy(toOverWritten, addr, cm->valuesz);
  }
}

void *cmap_get(const CMap *cm, const char *key)
{
  void *blob = getBlobFromKey(cm, key);
  return (blob)? getBlobValPtr(blob): NULL;
}

void cmap_remove(CMap *cm, const char *key)
{
  void *toRemove = getBlobFromKey(cm, key);
  if(cm->fn) (cm->fn)(getBlobValPtr(toRemove));
  char* ptrToFirstChar = getBlobKey(toRemove);
  ptrToFirstChar[0] = '\0';
  (cm->nelems)--;
} // the blob is still malloc'ed; will be diposed at the end of cmap_dipose
  
const char *cmap_first(const CMap *cm)
{
  if (cm->nelems == 0) return NULL;
  for(int i = 0; i < (cm->nbuckets); i++){
    if((cm->buckets)[i])
      return getBlobKey(cm->buckets[i]);
  }
  return NULL;
}

const char *cmap_next(const CMap *cm, const char *prevkey)
{
  void *prevBlob = getBlobFromKey(cm, prevkey);
  if (getNextBlob(prevBlob) && !isEmptyBlob(getNextBlob(prevBlob)))
    return getBlobKey(getNextBlob(prevBlob)); // returns next valid elem in the same list

  for (int i = hash(prevkey, cm->nbuckets) + 1; i < (cm->nbuckets); i++) {
    if ( (cm->buckets)[i] && !isEmptyBlob((cm->buckets)[i]) )
      return getBlobKey((cm->buckets)[i]);
  }

  return NULL; // end of the CMap
}

void cmap_dispose(CMap *cm)
{
  for(int i = 0; i < cm->nbuckets; i++) {
    if(cm->buckets[i])
      cleanBucket_i(cm, cm->buckets[i]);
  }
  free(cm->buckets);
  free(cm);
}

