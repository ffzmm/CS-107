/* File: average.c
 * ---------------
 * A test program to experiment with different ways of calculating the average
 * of a sequence of floating point numbers. You will mostly use the code as-is 
 * and run on the sequences we provide, but you are free to make changes to
 * this program for your own experiments. Try constructing sequences of your 
 * own or add a new variant of average and use them to further explore how 
 * floats work.
 */

#include <float.h>
#include <limits.h> 
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Comparator to order floats in increasing order
static int cmp_flt(const void *p1, const void *p2)
{
    float f = *(float *)p1, g = *(float *)p2;
    if (f < g) return -1;
    if (f > g) return 1;
    return 0;
}

// Comparator to order floats in decreasing absolute value order
static int rev_abs_cmp_flt(const void *p1, const void *p2)
{
    float f = fabsf(*(float *)p1), g = fabsf(*(float *)p2);
    if (f < g) return 1;
    if (f > g) return -1;
    return 0;
}

/* Function: average, averageA, averageB, averageC
 * -----------------------------------------------
 * Four different versions of average. One naive and
 * 3 "improved" versions.
 */

static float average(const float arr[], int n)
{
    float sum = 0;
    for (int i = 0; i < n; i++)
        sum += arr[i];
    return sum / n;
}

static float averageA(const float arr[], int n)
{
    float sum = 0;
    for (int i = 0; i < n; i++)
        sum += arr[i] / n;
    return sum;
}

static float averageB(const float arr[], int n)
{
    float copy[n];  // don't re-arrange caller's version of array, make copy
    memcpy(copy, arr, sizeof(copy));
    qsort(copy, n, sizeof(copy[0]), cmp_flt);
    float sum = 0;
    for (int i = 0; i < n; i++)
        sum += copy[i];
    return sum / n;
}

static float averageC(const float arr[], int n)
{
    float copy[n];  // don't re-arrange caller's version of array, make copy
    memcpy(copy, arr, sizeof(copy));
    qsort(copy, n, sizeof(copy[0]), rev_abs_cmp_flt);
    float sum = 0;
    for (int i = 0; i < n; i++)
        sum += copy[i];
    return sum / n;
}


void test_sequence(int id, float *arr, int count)
{
    float naive = average(arr, count), a = averageA(arr, count), b = averageB(arr, count), c = averageC(arr, count);
    printf("Sequence #%d average: naive = %.9g  a = %.9g  b = %.9g  c = %.9g\n", id, naive, a, b, c);
}

#define ARRAY_AND_COUNT(stackarr) stackarr, sizeof(stackarr)/sizeof(stackarr[0])

int main(int argc, char *argv[])
{
    float seq1[] = {4, 4.5, 5, 0};
    float seq2[] = {FLT_MAX, FLT_MAX};
    float seq3[] = {33333333, 66666666, 0};
    float seq4[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    float seq5[] = {66666666, 3, -66666666};
    float seq6[] = {1 << 24, 1, 0, -1};
    float seq7[] = {1.5e8, 100.5, -1.5e8, 99.5};

    test_sequence(1, ARRAY_AND_COUNT(seq1));
    test_sequence(2, ARRAY_AND_COUNT(seq2));
    test_sequence(3, ARRAY_AND_COUNT(seq3));
    test_sequence(4, ARRAY_AND_COUNT(seq4));
    test_sequence(5, ARRAY_AND_COUNT(seq5));
    test_sequence(6, ARRAY_AND_COUNT(seq6));
    test_sequence(7, ARRAY_AND_COUNT(seq7));
    return 0;
}

