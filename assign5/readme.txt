File: readme.txt
Author: Iris Yang 


0) Explain the tactics you used to suppress/avoid/disable explosions.
---------------------------------------------------------------------
I set a break point at explode_bomb to avoid executing it. When the program hits
that break point, I kill the program.



1) level_1 contains an instruction mov $<hex>,%edi
Explain how this instruction fits into the operation of level_1. What is this
hex value and for what purpose is it being moved?  Why does this instruction
reference %edi instead of the full %rdi register?
--------------------------------------------------------------------------------
This hex value is the address where the correct input string is stored. It is moved
into %edi to be passed as the 1st argument in the strcmp function. Since it has the
same size as a pointer (8 bytes), it only needs the lower 32 bits in the %rdi.



2) level_2 contains a call to sscanf, but it is not preceded by a mov into %rdi.
How then is the first argument being passed to sscanf?  What are the other
arguments being passed to this sscanf call?
--------------------------------------------------------------------------------
The first argument, which is read from line, is already saved in %rdi in the main function. 
Along with %rdi, the string format stored in %esi and two other arguments stored in
%rcx and %rdx are all passed to sscanf.



3) level_3 calls the binky function. Translate the assembly instructions for the
binky function into an equivalent C version.
-------------------------------------------------------------------------------
binky(int edi, int esi, int edx) {
  int r11d = 1;
  int r10d = 1;
  long int rax = 0; // wants to be 64 bits long
  int mask = (1<<8)-1; // 0xF
  int ecx = 0;
  int shift;
  
  while ((r11d - edi) <= 0 ) {
    ecx = edx - 1;
    long int r8 = 0;

    while (ecx >= 0) {
      long int r9 = (long int)r10d;
      shift = (ecx&mask);
      r9 = r9<<shift;
      r8 = (r9|r8);
      ecx--;
    }

    ecx = esi;
    shift = ecx&mask;
    rax = rax<<shift;
    rax += r8;
    r11d++;
  }

  return ~rax;
}   


4) level_4 declares a local variable that is stored on the stack at 0x8(%rsp).
What is the type/size of this variable? Explain how can you discern its type
from following along in the assembly, even though there is no explicit type
information in the assembly instructions. Within level_4 there is no instruction 
that writes to this variable. Explain how the variable is initialized (what value
it is set to and when/where does that happen?).
--------------------------------------------------------------------------------
0x8(%rsp) should be a char* which is 8 bytes. It was moved to %rdi before invoking
the strlen function, which takes one argument of type char*. It is initialized in
the after the "strtol" function call, and is set to be the pointer to the character
after the last number.


5) level_5 calls a function named count. What does does this function count? 
What are the parameters to this function? How is count used within level_5?
--------------------------------------------------------------------------------
This function counts the number of pairs that have the same last digit. There are
four arguments. The first one is an element  in the sorted array that is going to be 
compare with. The second one is a pointer to the starting element in the second array
which is to be compared with the first argument. The third argument is the number of
elements in the second array to be examined, and finally the fourth one is a callback
function 'fn'. It tests if the number of pairs with the same last digit is 5 excluding
the first pair.
