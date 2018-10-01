/*
 * File: allocator.c
 * Author: YOUR NAME HERE
 * ----------------------
 * A trivial allocator. Very simple code, heinously inefficient.
 * An allocation request is serviced by incrementing the heap segment
 * to place new block on its own page(s). The block has a pre-node header
 * containing size information. Free is a no-op: blocks are never coalesced
 * or reused. Realloc is implemented using malloc/memcpy/free.
 * Using page-per-node means low utilization. Because it grows the heap segment
 * (using expensive OS call) for each node, it also has low throughput
 * and terrible cache performance.  The code is not robust in terms of
 * handling unusual cases either.
 *
 * In short, this implementation has not much going for it other than being
 * the smallest amount of code that "works" for ordinary cases.  But
 * even that's still a good place to start from.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "allocator.h"
#include "segment.h"
#include <math.h>
#include <float.h>

// Heap blocks are required to be aligned to 8-byte boundary
#define ALIGNMENT 8
#define BUCKETNUMBER 13 // number of buckets 
#define SWORD 4 // size of word
#define MIN(X, Y) (((X) <= (Y)) ? (X) : (Y))
static void *arr_of_list[BUCKETNUMBER]; // array of linked list
static void *hpptr;
static int numpages;

typedef struct {
    int hdrsz;   // header contains just one 4-byte field
} headerT;

// Very efficient bitwise round of sz up to nearest multiple of mult
// does this by adding mult-1 to sz, then masking off the
// the bottom bits to compute least multiple of mult that is
// greater/equal than sz, this value is returned
// NOTE: mult has to be power of 2 for the bitwise trick to work!
static inline size_t roundup(size_t sz, size_t mult)
{
    return (sz + mult-1) & ~(mult-1);
}

// Given a pointer to block header, advance past
// header to access start of payload
static inline void *payload_for_hdr(headerT *header)
{
    return (char *)header + sizeof(headerT);
}

static inline void set_status(headerT *ptr, int status) // ptr is a pointer to header
{
    *(unsigned int *)ptr = ((*(unsigned int *)ptr) & (~1)) + status;
//    ((char *)ptr)[ALIGNMENT * SWORD - 1] = status;
}

// return the allocation status (1:allocated; 0:free)
static inline int get_status(void *ptr)
{
    return ((*(unsigned int *)ptr) & 1);
    // (int)(((char *)ptr)[ALIGNMENT * SWORD - 1]);
}

// return a pointer to the <prev> section in current block
static inline void *get_prev(void *ptr) // ptr points to header of free block
{
    return (char *)ptr + sizeof(headerT);
}

// return a pointer to the <succ> section in current block
static inline void *get_succ(void *ptr) // ptr points to header of freee block
{
    return (char *)ptr + sizeof(headerT) + sizeof(void *);
}

// set the <prev> section of current block 
static inline void set_prev(void *ptr, void *prev) //ptr to header of free block, prev is a pointer to somewhere else(prev block's header or the head of linked list).
{
    *(void **)get_prev(ptr) = prev;
}

// set the <succ> section of current block 
static inline void set_succ(void *ptr, void *succ) // ptr to header of free block
{
    *(void **)get_succ(ptr) = succ;
}

// use bitmask to obtain the block size in header
static inline int get_blocksz(void *ptr) // ptr is a pointer to header 
{
    return (*(unsigned int *)ptr) & (~0x7);
}

// given block size, find the most suitible index 
// our arr of linked list is: {1}, {2,3}, {4,5,6,7}, {8,...,15}, {16,...,31}, ..., {2^13,...,2^14-1}
static inline int find_index(int blocksz) 
{
    return MIN((int)(log2f((float)(blocksz / ALIGNMENT))), BUCKETNUMBER - 1); //EFFICIENCY
}

// insert a free block to the arr of linked list
static void insert(void *header) 
{
    size_t blocksz = get_blocksz(header);
    int index = find_index(blocksz); // find the index to insert
    // insert to the front of the linked list
    if (arr_of_list[index] != 0) { // if not an empty linked list
        set_succ(header, arr_of_list[index]);
        set_prev(arr_of_list[index], header);
        set_prev(header, &arr_of_list[index]);
        arr_of_list[index] = header;     
    } else { // empty list before
        set_succ(header, NULL);
        set_prev(header, &arr_of_list[index]);   
        arr_of_list[index] = header;
    }
    printf("In insert...\n");
    validate_heap();
}

//passed in header pointer and the index it belongs to, delete it from the free list
static void delete(void *header, int index) 
{
    // for (void *curr = arr_of_list[index]; curr != NULL; curr = *(void **)(get_succ(curr))) {
    //     printf("curr address = %p; curr = %d", curr, *(unsigned int *)curr);
    // }
    // printf("-------------------------------------------------------\n");
    void *prev = *(void **)(get_prev(header));
    void *succ = *(void **)(get_succ(header));
    if (header == arr_of_list[index]) { // first block of this linked list
        if (succ != NULL) { // is first, not the last
            arr_of_list[index] = succ;
            set_prev(succ, prev);
        } else { // the only block in this list
            arr_of_list[index] = NULL;
        }
    } else if (succ == NULL) { // not first, but the last
        set_succ(prev, succ);
    } else { // neither, nor
        set_succ(prev, succ);
        set_prev(succ, prev);       
    }
    printf("In delete... \n");
    validate_heap();
}

// Given a pointer to start of payload, simply back up
// to access its block header
static inline headerT *hdr_for_payload(void *payload)
{
    return (headerT *)((char *)payload - sizeof(headerT));
}

// Given a pointer to start of payload, simply back up
// to access its block footer
static inline headerT *ftr_for_payload(void *payload)
{
    size_t blocksz = get_blocksz(hdr_for_payload(payload)); // get blocksize
    return (headerT *)((char *)payload + blocksz - 2 * sizeof(headerT));
}

static void construct_block(void *ptr, int blocksz, int status) // ptr is pointer to header
{
    unsigned int header = blocksz + status;
    *(unsigned int *)ptr = header; // make header
    *(unsigned int *)((char *)ptr + blocksz - sizeof(headerT)) = header; // make footer
} 

// coalesce and change header and footer
static void *coalesce(void *ptr) // ptr is pointer to header of a block
{

    int size = get_blocksz(ptr);
    int old_size = size;
    int prev_alloc; // physically prev
    int succ_alloc;// physically succ

    // if ptr is now on the top most of the heap
    if (ptr == hpptr && numpages * PAGE_SIZE >= size) { 
          succ_alloc = get_status((char *)ptr + size);
          if (!succ_alloc) { // only react when physical succ is free
            size += get_blocksz((char *)ptr + size); // change size
            // change blocksz in header and footer of the big block
            *(unsigned int *)ptr = size; // header
            *(unsigned int *)((char *)ptr + size - sizeof(headerT)) = size; // footer    
           }
        return ptr;
    } else if ( ptr != hpptr && numpages * PAGE_SIZE == size) { 
    // if ptr is pointing to the last most block on heap
        prev_alloc = get_status((char *)ptr - sizeof(headerT));
        if (!prev_alloc) {
            size += get_blocksz((char *)ptr - sizeof(headerT)); // change size
            ptr = (char *)ptr - get_blocksz((char *)ptr - sizeof(headerT)); // change ptr to heaeder of physical prev (big block header)
            // change blocksz in header and footer of the big block
            *(unsigned int *)ptr = size;
            *(unsigned int *)((char *)ptr + size - sizeof(headerT)) = size;
        }
        return ptr;
    } else if (ptr == hpptr && numpages * PAGE_SIZE == size) {return ptr; } // the only block on the heap

    // normal case
    succ_alloc = get_status((char *)ptr + size);
    prev_alloc = get_status((char *)ptr - sizeof(headerT));
    // ptr is pointing to a normal block (not the first or last)
    if (prev_alloc && succ_alloc) {
        return ptr;
    } else if (prev_alloc && !succ_alloc) { // prev allocted, succ free
        size += get_blocksz((char *)ptr + size);
        // change blocksz in header and footer of the big block
        *(unsigned int *)ptr = size;
        *(unsigned int *)((char *)ptr + size - sizeof(headerT)) = size;     
    } else if (!prev_alloc && succ_alloc) { // prev free, succ allocted
        size += get_blocksz((char *)ptr - sizeof(headerT));
        ptr = (char *)ptr - get_blocksz((char *)ptr - sizeof(headerT)); // change ptr
        // change blocksz in header and footer of the big block
        *(unsigned int *)ptr = size;
        *(unsigned int *)((char *)ptr + size - sizeof(headerT)) = size;
    } else { // prev and succ both free
        size = size + get_blocksz((char *)ptr + size) + get_blocksz((char *)ptr - sizeof(headerT));
        ptr = (char *)ptr - get_blocksz((char *)ptr - sizeof(headerT)); // change ptr
        *(unsigned int *)ptr = size;
        *(unsigned int *)((char *)ptr + size - sizeof(headerT)) = size;
    }
    return ptr;
}

static void split_n_insert(void *ptr, int blocksz, int size) // size is size needed (rounded up version)
{
    validate_heap();
    int size1 = size;
    int size2 = blocksz - size;
    if (size2 < 3 * ALIGNMENT) { // no need to split if have less than 3 * 8bytes left
        size1 = blocksz;
        construct_block(ptr, size1, 1);
    } else { // split
        construct_block(ptr, size1, 1);
        construct_block((char *)ptr + size1, size2, 0);
        insert((char *)ptr + size1);
    }
}

static void *find_fit_index(int size, int index) 
{
    void *curr = arr_of_list[index];
    while (curr != NULL) { // not an empty linked list
        // compare size with blocksz
        // printf("one round in while loop in find_fit_index %d, we want %d, now blocksize = %d\n", index, size, get_blocksz(curr));
        if (size <= get_blocksz(curr)) {
            // printf("now delete ....\n");
            delete(curr, index);
            // printf("now split_n_insert ....\n");
            split_n_insert(curr, get_blocksz(curr), size);
            // printf("now return ....\n");
            return curr;
        }
        curr = *(void **)(get_succ(curr));
    }
    // not found
    return NULL;
}

static void *find_fit(int size)
{
    int index = find_index(size);
    void *fit = NULL;
    // find in current index
    fit = find_fit_index(size, index);
    if (fit != NULL) return fit;
    // find in other greater indexes
    for (int i = index +1; i < BUCKETNUMBER; i++) {
        fit = find_fit_index(size, i);
        if (fit != NULL) return fit;
    }
    int sz = 0;
    // bool chk_free = 0;
    void *ptr = NULL;
    //check if the physically last block is free
    if (numpages != 0){
        if (get_status((headerT *)(((char *)hpptr + numpages * PAGE_SIZE) - sizeof(headerT))) == 0) {
            sz = get_blocksz((headerT *)(((char *)hpptr + numpages * PAGE_SIZE) - sizeof(headerT)));
            ptr = (char *)hpptr + numpages * PAGE_SIZE - sz; // point to header of the last free block
        }   
    }
    // calculate the number of pages needed
    int npages = 1;
    if (index == BUCKETNUMBER - 1) {
        npages = (size - sz)/PAGE_SIZE + 1;
    }

    //extend heap
    fit = extend_heap_segment(npages); 
    if (fit != NULL) {
        numpages += npages;
        int blocksz = npages * PAGE_SIZE;
        if (ptr != NULL) { // last block from before is free
            fit = ptr;
            blocksz += sz;
        } 
        printf("---------------blocksz = %d; numpages = %d----------\n", blocksz, numpages);
        split_n_insert(fit, blocksz, size);      
    }
    return fit; // pointer to header of fitted block
}
/* The responsibility of the myinit function is to configure a new
 * empty heap. Typically this function will initialize the
 * segment (you decide the initial number pages to set aside, can be
 * zero if you intend to defer until first request) and set up the
 * global variables for the empty, ready-to-go state. The myinit
 * function is called once at program start, before any allocation 
 * requests are made. It may also be called later to wipe out the current
 * heap contents and start over fresh. This "reset" option is specifically
 * needed by the test harness to run a sequence of scripts, one after another,
 * without restarting program from scratch.
 */
bool myinit()
{
    hpptr = init_heap_segment(0); // reset heap segment to empty, no pages allocated
    // arr_of_list = {0}; // initialize arr of linked lists
    numpages = 0;
    return true;
}

void *mymalloc(size_t requestedsz)
{
    size_t size = roundup(requestedsz + 2 * sizeof(headerT), ALIGNMENT); // round up 
    if (size < 3*ALIGNMENT) size = 3*ALIGNMENT;
    void *fit = find_fit(size); // only worry about extend page here in find_fit function
    // fit might be NULL because exotend_heap might return NULL
    return (fit != NULL) ? payload_for_hdr(fit) : NULL; 
    // headerT *header = extend_heap_segment(npages);
    // header->payloadsz = requestedsz;
    // return payload_for_hdr(header);
}

void myfree(void *ptr)
{
    if (ptr != NULL) { 
        void *header = hdr_for_payload(ptr);
        set_status(header, 0); // set allocation status in header
        set_status(ftr_for_payload(ptr), 0); // set allocation status in Footer
        header = coalesce(header); 
        insert(header);
    }
    // if ptr points to NULL, do nothing
}


// realloc built on malloc/memcpy/free is easy to write.
// This code will work ok on ordinary cases, but needs attention
// to robustness. Realloc efficiency can be improved by
// implementing a standalone realloc as opposed to
// delegating to malloc/free.
void *myrealloc(void *oldptr, size_t newsz) //EFFICIENCY
{
    size_t size = roundup(newsz + 2 * sizeof(headerT), ALIGNMENT); // new block size
    void *newptr = oldptr;
    if (oldptr == NULL) { // Special_Case_1: oldptr == NULL. Same as malloc
        newptr = mymalloc(newsz);
    } else { // valid oldptr
        if (newsz == 0) { // Special_Case_2: newsz == 0. Almost same as free, except that this function returns NULL
            myfree(oldptr);
            newptr = mymalloc(newsz); // make a pointer that can be passed to free
        } else { // Normal_Case
            size_t blocksz = get_blocksz(hdr_for_payload(oldptr)); // old block size
            if (size > blocksz) { // if need a bigger block
                newptr = mymalloc(newsz);
                if (newptr != NULL) {
                    memcpy(newptr, oldptr, (blocksz - 2 * sizeof(headerT)));
                    myfree(oldptr);
                } // else malloc failed do nothing and return NULL(newptr) in the end
            } // else no need to do anything
        }
    }
    return newptr;
}


// validate_heap is your debugging routine to detect/report
// on problems/inconsistency within your heap data structures
bool validate_heap()
{
    for (int i = 0; i < BUCKETNUMBER; i++) {
      int count = 0;
      void * curr = arr_of_list[i];
      while (curr != NULL) {
        count++;
        printf("Bucket %d: block size: %d\n", i, get_blocksz(curr));
        curr = *(void**)get_succ(curr);
      }
      printf("%d in bucket %d\n", count, i);
    } 
    return true;
}

