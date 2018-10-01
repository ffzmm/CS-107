/*
 * File: reassemble.c
 * Author: Cynthia Lee, Travis Geis, and Julie Zelenski
 * ----------------------
 * This program reassembles data fragments using a greedy match-and-merge
 * algorithm.
 * 
 * STUDENTS: You should edit this file to complete the implementation of
 * this program. You do not need to (and shouldn't) edit the functions that
 * are already complete in this file (main, read_one_fragment, etc.), but you
 * may want to read them to see how they work, since you'll need to follow
 * them when you are debugging. In this file, you should provide implementations
 * of the functions declared in reassemble.h. You are alslo welcome to add any
 * other helper functions you would like, to further decompose the problem.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include "reassemble.h"

// limits on maximum fragment length and maxinum number of frags in a file
#define MAX_FRAG_LEN 1000
#define MAX_FRAG_COUNT 5000

// Local function declarations 
// STUDENTS: add your helper function declarations here--for those functions that 
// you add that are not already decclared in reassemble.h.
int read_all_fragments(FILE *fp, char *arr[], int maxFrags);
char *read_one_fragment(FILE *fp, int fragCountSoFar);

// helper function that merges two frags w/ longest overlaps when there are remainingElems left
int do_reassemble_helper(char * frags[], int remainingElems);

// Function definitions 

/* Function: main
 * Implement the reassembly program, accepting a file name from the command line
 * and printing the final reassembly result.
 * 
 * STUDENTS: DO NOT EDIT THIS FUNCTION.
 */
int main(int argc, const char *argv[])
{
  char *frags[MAX_FRAG_COUNT];
  FILE *fp;

  if (argc < 2){ // remember, argv[0] is command name, so argc should be at least 2.
    fp = stdin;
    printf("Enter fragments to reassemble in format {abc}{cde}. End with Control-D on a line by itself.\n\n");
  } else {
    fp = fopen(argv[1], "r");
    if (!fp){
      printf("reassemble: cannot open file \"%s\"\n", argv[1]);
      exit(1);
    }
  }

  int count = read_all_fragments(fp, frags, MAX_FRAG_COUNT);

  if (fp != stdin) fclose(fp); // close the file if we opened one

  // Now reassemble!
  if (count > 0) {
    do_reassemble(frags, count);
  }
  
  // clean_frags(frags, count);
  return 0;
}

void do_reassemble(char * frags[], int nelems) {
   int remainingElem = nelems;
   if (nelems <= 1) {
      printf("%s\n",frags[0]);
      return;
   }

   int finalStringLocation = 0; //stores the location of final merged string
   while(remainingElem > 1) {
     finalStringLocation = do_reassemble_helper(frags, remainingElem);
     remainingElem --;
   }
   printf("%s\n", frags[finalStringLocation]);
   free(frags[finalStringLocation]); // frees the last superstring at the very end
}

/* Function: do_reassemble_helper merge two strings with longest overlaps in a single round 
within #remainingElem frags, so that the # of remaining fragments is reduced by 1 
(completed in do_reassemble). nelem is passed to keep track of the end of the frags[]. 
*/
int do_reassemble_helper(char * frags[], int remainingElem)
{

   // if(remainingElem <= 1) return;

   int maxOverlap = -1; // so that two strings w/o overlaps can also be caught
   int indOverlap = -1; // index of the overlap location
   char *left = NULL; // ptr to the left frag to be merged
   char *right = NULL; // prt to the right frag to be merged
   int indSupString = -1; // index where the ptr to the merged string will be stored
   int indNull = -1; // index where the ptr to the last element in frags[] is copied over

   for(int i = 0; i < remainingElem; i++) {
      for (int j = i+1; j < remainingElem; j++) {
            char *a = frags[i];
            char *b = frags[j];
            int *tempOverlap1 = malloc(sizeof(int));
            int *tempOverlap2 = malloc(sizeof(int));  
	    int OL_left = locate_overlap(a, b, tempOverlap1);
	    int OL_right = locate_overlap(b, a, tempOverlap2);
	    if (OL_left >= OL_right && OL_left > maxOverlap) {
	       left = a;
	       right = b;
	       maxOverlap = OL_left;
	       indOverlap =  *tempOverlap1;
	       indSupString = i;
	       indNull = j;
	    } else if (OL_right > OL_left && OL_right > maxOverlap) {
	       left = b;
	       right = a;
	       maxOverlap = OL_right;
	       indOverlap = *tempOverlap2;
	       indSupString = j;
	       indNull = i;
	    }
            free(tempOverlap1);
            free(tempOverlap2);
      } // end of j     
   } // end of i
   
   char *superString = merge(left, right, indOverlap);
   frags[indSupString] = superString;
   frags[indNull] = frags[remainingElem-1];
   return indSupString;
}

// STUDENTS: Complete this function. (see reassemble.h for details)
int locate_overlap(char *left_frag, char *right_frag, int *offset)
{
   if (strlen(left_frag) == 0 || strlen(right_frag) == 0) { //guard case: either string is empty
      *offset = strlen(left_frag);
      return 0;
   }

   if (strlen(right_frag) <= strlen(left_frag)) { // case 1 and 2: right string is a substring of (or identical to) left string
      if (strstr(left_frag, right_frag) != NULL) {
         char *temp = strstr(left_frag, right_frag);
         *offset = strlen(left_frag) - strlen(temp);
         return strlen(right_frag);
      }
   }
   for (int i = 0; i < strlen(left_frag); i++) { // case 3: right string overlaps and concatenates to the left string
         if (match_at_offset(left_frag, right_frag, i)){
            *offset = i;
            return strlen(left_frag)-i;
         }
   }

   *offset = strlen(left_frag); // else case: no overlaps
   return 0;
}

// STUDENTS: Complete this function. (see reassemble.h for details)
bool match_at_offset(char *left_frag, char *right_frag, int offset)
{
   if (offset <= 0 || offset >= strlen(left_frag) || offset + strlen(right_frag) <= strlen(left_frag)) {
      return false;
   }
   return (strncmp(left_frag + offset, right_frag, strlen(left_frag)-offset) == 0);   
}

// STUDENTS: Complete this function. (see reassemble.h for details)
char *merge(char *left_frag, char *right_frag, int offset)
{
   char *merged_string = malloc(sizeof(char)*(strlen(left_frag) + strlen(right_frag) + 1)); // add'l 1 for \0
   strcpy(merged_string, left_frag);   
   strcpy(merged_string + offset, right_frag);
   free(left_frag);
   free(right_frag);
   return merged_string;
}

/* Function: clean_frags
cleans up heap allocation created by char *frags[]
*/
void clean_frags(char* frags[], int nelems) {
   for(int i = 0; i < nelems; i++) {
      free(frags[i]);
   }
}
/* Function: read_all_fragments
 * --------------------------
 * Reads a sequence of correctly-formed fragments from the opened FILE *
 * until EOF or maximum number read (whichever comes first). The strings are
 * duplicated into the heap and pointers stored into the array. The total
 * number of strings read is returned from the function.
 *
 * If read_one_fragment encounters an error, read_all_fragments returns
 * the number of fragments successfully read multiplied by -1.
 * 
 * STUDENTS: DO NOT EDIT THIS FUNCTION.
 */
int read_all_fragments(FILE *fp, char *arr[], int max_frags)
{
  int count = 0;
  for (int i = 0; i < max_frags; i++) {
    char *frag = read_one_fragment(fp, count);
    if (!frag){ // indicates NULL, which means no more fragments
      return count;
    }
    arr[i] = frag;
    count++;
  }
  // By this point, we've read as many frags as we can accept.
  return max_frags;
}

/* Function: read_one_fragment
 * -------------------------
 * Reads a single correctly-formed fragment from the opened FILE * and returns
 * pointer to a dynamically-allocated string of characters. Returns NULL
 * if fragment could not be successfully scanned (malformed or EOF).
 *
 * It takes as an argument the number of fragments successfully read so far, in
 * order to use it in error messages in case of fragment format/read error.
 * 
 * STUDENTS: DO NOT EDIT THIS FUNCTION.
 */
char *read_one_fragment(FILE *fp, int frag_count_so_far)
{
  char start, end;
  char buffer[MAX_FRAG_LEN + 1]; // temporary buffer on stack; the +1 is for null-terminator
  int scan_result = 0;

  // The space at the start of the format string means we allow arbitrary white space
   scan_result = fscanf(fp, " %c%1000[^{}]%c", &start, buffer, &end);

  if (scan_result == 3) {
    // fscanf filled `start`, `end`, and `buffer` according to our pattern
    if (start != '{') {
      error(1, 0, "input format error. read %d well-formed fragments, next fragment doesn't open with {", frag_count_so_far);
    }
    if (end != '}') {
      error(1, 0, "input format error. read %d well-formed fragments, next fragment is longer than 1000 characters", frag_count_so_far);
    }
    // `start` and `end` were correct, so we know this fragment is good.
    return strdup(buffer); // return persistent heap copy, perfectly sized
  } else if (scan_result == 2) {
    // `start` and `buffer` are filled, but `end` is undefined
    error(1, 0, "input format error. read %d well-formed fragments, next fragment doesn't close with }", frag_count_so_far);
    return NULL;
  } else if (scan_result == 1) {
    // Only `start` is valid here
    printf("%c", start);
    error(1, 0, "input format error. read %d well-formed fragments, next fragment is malformed/empty", frag_count_so_far);
    return NULL;
  } else {
    return NULL; // nothing left.
  }
}



