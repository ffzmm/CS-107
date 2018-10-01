/*
 * File: bits.c
 * Author: Iris Yang 
 * ----------------------
 *
 */

#include "bits.h"
#include <assert.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

/* The NOT_YET_IMPLEMENTED macro is used as the body for all functions
 * to remind you about which operations you haven't yet implemented.
 * It wil report an error if a call is made to an not-yet-implemented
 * function (this is preferable to returning garbage or silently
 * ignoring the call).  Remove the call to this macro as you implement
 * each function and finally you can remove this macro and comment
 * when no longer needed.
 */
#define NOT_YET_IMPLEMENTED printf("%s() not yet implemented!\n", __func__); raise(SIGKILL); exit(107);
/*
static void printBits(size_t const size, void const *const* ptr)
{
  unsigned char *b = (unsigned char*) ptr;
  unsigned char byte;
  int i, j;

  for (i = size-1; i >= 0; i--) {
    for (j = 7; j >= 0; j --) {
      byte = (b[i] >> j) & 1;
      printf("%u", byte);
    }
  }
  puts("");
}
*/

int cmp_bits(int a, int b)
{
  int count_a = 0;
  int count_b = 0;
  for (int i = 0; i < 8*sizeof(int); i++) {
    int mask = 1<<i;
    if((a&mask) == mask) count_a++;
    if((b&mask) == mask) count_b++;
  } 

  if (count_a > count_b) 
    return 1;
  else if (count_a < count_b) 
    return -1;
  else 
    return 0;
}

unsigned short make_set(int values[], int nvalues)
{
  unsigned short bit_vec = 0;
  for (int i = 0; i < nvalues; i++) {
    int toShift = values[i];
    bit_vec += (1<<toShift);
  }
  return bit_vec;
}

bool is_single(unsigned short used_in_row, unsigned short used_in_col, unsigned short used_in_block)
{
  unsigned short allUsed = used_in_row | used_in_col | used_in_block;
  unsigned short mask = ((1<<9)-1)<<1; // make a mask to select only 9 bits
  unsigned short notUsed = (~allUsed) & mask; // is_single if only one 1 is present
  // printf("%x\n", notUsed);
  if (notUsed == 0) return false;
  return ((notUsed&(notUsed-1)) == 0);
}

utype sat_add_unsigned(utype a, utype b)
{
  utype leftmostDig = 1 <<(8*sizeof(utype)-1);
  utype max = ~0;
  utype tempSum = a + b;

  if( (a&leftmostDig) != (b&leftmostDig) ) {// one 1 and one 0 at msb; overflow may occur 
    if ( (tempSum&leftmostDig) == 0 ) //  overflow occurs; msb flips sign
      return max;
  } 
  if( ((a&leftmostDig) == leftmostDig) && ((b&leftmostDig) == leftmostDig) ) // both 1 at most sig bit
    return max;
  return tempSum;
}

stype sat_add_signed(stype a, stype b)
{
  stype signDig = 1<<(8*sizeof(stype)-1); // also the INT_MIN
  stype max = signDig - 1;
  stype tempSum = a + b;

  if ( ((signDig&a) == signDig) && ((signDig&b) == signDig) ){ // both -
    if ( (tempSum&signDig) == 0) // overflows; sigDign flips to 0
      return signDig;
  } else if ( ((signDig&a) == 0) && ((signDig&b) == 0) ) { // both +
    if ((signDig&tempSum) == signDig) // overflows; signDig flips to 1
      return max;
  }
  return tempSum; // one + one -; no saturation possible
}

#define MAX_BYTES 5

/* Function: print_hex_bytes
 * --------------------------
 * Given the pointer to a sequence of bytes and a count, and this
 * function will print count number of raw bytes in hex starting at
 * the bytes address, filling in with spaces to align at the width
 * of the maximum number of bytes printed (5 for push).
 */
void print_hex_bytes(const unsigned char *bytes, int nbytes)
{
    for (int i = 0; i < MAX_BYTES; i++)
        if (i < nbytes)
            printf("%02x ", bytes[i]);
        else
            printf("  ");
    printf("\t");
}

// Each register name occupies index of array according to number
const char *regnames[] = {"%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi"};


void disassemble(const unsigned char *raw_instr)
{
  char firstByte = raw_instr[0];
  char mask = 0xf0; // extract leftmost 4 bits in the first byte
  char firstHalfByte = firstByte&mask;

  if ( firstHalfByte == 0x50) { // 1 byte
    print_hex_bytes(raw_instr, 1);
    mask = (1<<2)^(1<<1)^1;
    char reg = mask&firstByte;
    printf("pushq %s\n", regnames[(size_t)reg]);

  } else if (firstByte == 0x68 ) { // 5 bytes
    print_hex_bytes(raw_instr, 5);    
    unsigned int mem = (raw_instr[4]<<24)^(raw_instr[3]<<16)^(raw_instr[2]<<8)^raw_instr[1];
    printf ("pushq $0x%x\n", mem);

  } else { // fistByte = 0xff
    mask = (1<<2)^(1<<1)^1; // mask for reg
    char secondByte = raw_instr[1];
    char reg = mask&secondByte;

    if (reg == 0x2) { // 2 bytes
      print_hex_bytes(raw_instr, 2);
      printf("pushq (%s)\n", regnames[(size_t)reg]);

    } else if (reg == 0) { // 3 bytes
      char mem = raw_instr[2];
      print_hex_bytes(raw_instr, 3);
      printf("pushq 0x%x(%s)\n", mem, regnames[(size_t)reg]);  

    } else { // 4 bytes
      char mask_scale = (1<<6)^(1<<7); // select the scale factor
      char scale = (mask_scale&raw_instr[2])>>6;
      int scale_int = -1;
      switch (scale) {
        case 0:
          scale_int = 1; break;
        case 1:
          scale_int = 2; break;
        case 2:
          scale_int = 4; break;
        case 3:
          scale_int = 8; break;
        default:
          printf("error\n"); 
      }
      char regl = mask&raw_instr[2]; // left reg
      char regr = ((mask<<3)&raw_instr[2])>>3; // right reg
      print_hex_bytes(raw_instr, 4);
      printf("pushq 0x%x(%s,%s,%d)\n", raw_instr[3], regnames[(size_t)regl], regnames[(size_t)regr], scale_int);
    }
  } 
}
