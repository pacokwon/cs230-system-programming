/*
 * CS:APP Data Lab
 *
 * Kwon Haechan 20190046
 *
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:

  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code
  must conform to the following style:

  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>

  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.


  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function.
     The max operator count is checked by dlc. Note that '=' is not
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 *
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* We do not support C11 <threads.h>.  */
/*
 * bitXor - x^y using only ~ and &
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  return ~(~x & ~y) & ~(x & y);
}
/*
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
    int nOffset = n << 3;
    int mOffset = m << 3;

    int nMask = (0xFF << nOffset);
    int mMask = (0xFF << mOffset);

    int msb = 0x1 << 31;
    int nthByte = ((x & nMask) >> nOffset) & ~((msb >> nOffset) << 1);
    int mthByte = ((x & mMask) >> mOffset) & ~((msb >> mOffset) << 1);

    return (x & ~(nMask | mMask)) | (nthByte << mOffset | mthByte << nOffset);
}

/*
 * rotateLeft - Rotate x to the left by n
 *   Can assume that 0 <= n <= 31
 *   Examples: rotateLeft(0x87654321,4) = 0x76543218
 *   Legal ops: ~ & ^ | + << >> !
 *   Max ops: 25
 *   Rating: 3
 */
int rotateLeft(int x, int n) {
    int msb = 0x1 << 31;
    int mask = (msb >> n) << 1;

    int sub = 33 + ~n;
    int subMask = ~((msb >> sub) << 1);

    return (x & ~mask) << n | ((x & mask) >> sub & subMask);
}

/*
 * leftBitCount - returns count of number of consective 1's in
 *     left-hand (most significant) end of word.
 *   Examples: leftBitCount(-1) = 32, leftBitCount(0xFFF0F0F0) = 12
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 50
 *   Rating: 4
 */
int leftBitCount(int x) {
    int n = 0;
    int msb = 1 << 31;
    int half;

    x = ~x;
    half = !(x & (msb >> 15)) << 4;
    n += half;
    x <<= half;

    half = !(x & (msb >> 7)) << 3;
    n += half;
    x <<= half;

    half = !(x & (msb >> 3)) << 2;
    n += half;
    x <<= half;

    half = !(x & (msb >> 1)) << 1;
    n += half;
    x <<= half;

    half = !(x & msb);
    n += half;
    x <<= half;

    half = !(x & msb);

    n += half;

    return n;
}

/*
 * absVal - absolute value of x
 *   Example: absVal(-1) = 1.
 *   You may assume -TMax <= x <= TMax
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 10
 *   Rating: 4
 */
int absVal(int x) {
    int absMask = x >> 31;
    x = absMask ^ x;
    x = x + (~absMask + 1);
    return x;
}

/*
 * TMax - return maximum two's complement integer
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmax(void) {
    return ~(0x1 << 31);
}

/*
 * fitsShort - return 1 if x can be represented as a
 *   16-bit, two's complement integer.
 *   Examples: fitsShort(33000) = 0, fitsShort(-32768) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int fitsShort(int x) {
    int msbHalf = x >> 15;
    int half = x >> 16;

    int firstHalfNotUniform = x >> 31 ^ half;
    int boundaryBitsDifferent = (msbHalf ^ half) & 0x1; // (msbHalf & 0x1) ^ (half & 0x1)

    return !(firstHalfNotUniform | boundaryBitsDifferent); // !firstHalfNotUniform & !boundaryBitsDifferent
}

/*
 * rempwr2 - Compute x%(2^n), for 0 <= n <= 30
 *   Negative arguments should yield negative remainders
 *   Examples: rempwr2(15,2) = 3, rempwr2(-35,3) = -3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int rempwr2(int x, int n) {
    int mask = (1 << n) + ~0; // 0 ..(32 - n).. 01 ..(n).. 1
    int masked = x & mask; // mask front n bits
    int zeroMask = ~(!!masked) + 1; // 111...111 if not zero, 000...000 if zero
    int msbFilled = x >> 31;

    return (zeroMask & (msbFilled & ~mask)) | masked;
}

/*
 * sign - return 1 if positive, 0 if zero, and -1 if negative
 *  Examples: sign(130) = 1
 *            sign(-23) = -1
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 10
 *  Rating: 2
 */
int sign(int x) {
    int isNotZero = !!x; // all zero if x is zero, all 1 otherwise
    int msbFilled = x >> 31; // -1 if x is negative, 0 if x is positive or 0

    // if msb is -1, return -1
    // if msb is 0, return msb + isNotZero
    return msbFilled | (~msbFilled & (msbFilled + isNotZero));
}

/*
 * isNonNegative - return 1 if x >= 0, return 0 otherwise
 *   Example: isNonNegative(-1) = 0.  isNonNegative(0) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 3
 */
int isNonNegative(int x) {
    return !(x >> 31);
}

/*
 * isGreater - if x > y  then return 1, else return 0
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
    int signDiff = x ^ y; // msb denotes sign difference

    int signSameAndXGreater = ~(signDiff | (~y + x)); // msb 1 when signs are same AND x > y
    int signDiffAndYNegative = y & signDiff; // msb 1 when signs are different AND y is negative

    return ((signDiffAndYNegative | signSameAndXGreater) >> 31) & 0x1;
}

/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
    // basically, it's 33 - leftBitCount(x ^ msbMask)

    int msbMask = ~(x >> 31);
    int n = 0;
    int msb = 1 << 31;
    int half;

    x = ~(x ^ msbMask);

    half = !(x & (msb >> 15)) << 4;
    n += half;
    x <<= half;

    half = !(x & (msb >> 7)) << 3;
    n += half;
    x <<= half;

    half = !(x & (msb >> 3)) << 2;
    n += half;
    x <<= half;

    half = !(x & (msb >> 1)) << 1;
    n += half;
    x <<= half;

    half = !(x & msb);
    n += half;
    x <<= half;

    half = !(x & msb);

    n += half;

    return 34 + ~n;
}

/*
 * float_abs - Return bit-level equivalent of absolute value of f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument..
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_abs(unsigned uf) {
    unsigned int msbMasked = uf & 0x7FFFFFFF;

    if (msbMasked > 0x7F800000)
        return uf;
    else
        return msbMasked;
}

/*
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
    unsigned int signMask = 0x80000000u;
    unsigned int expMask = 0x7F800000u;
    unsigned int fracMask = 0x007FFFFFu;

    const int INT_MIN = 0x80000000;

    unsigned int sign = uf & signMask;
    unsigned int msbMasked = uf & 0x7FFFFFFF;
    int exponent = ((uf & expMask) >> 23) - 127;
    int fraction = (uf & fracMask) | (1 << 24);
    unsigned int rounded = fraction << (8 + exponent);

    if (msbMasked >= expMask)
        return 0x80000000u;

    if (exponent < 0)
        return 0;

    if (exponent >= 31)
        return INT_MIN;

    if (exponent > 24) return fraction << (exponent - 24);

    fraction = fraction >> (24 - exponent);
    if (rounded > 0x80000000u)
        fraction = fraction + 1;
    else if (rounded < 0x80000000u)
        fraction = fraction;
    else
        fraction = fraction + fraction & 0x1;

    if (sign)
        return -fraction;
    else
        return fraction;
}

/*
 *float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
    unsigned int expMask = 0x7F800000u;
    unsigned int zeroMask = 0x7FFFFFFFu;
    unsigned int fracMask = 0x007FFFFFu;
    unsigned int msbMask = 0x80000000u;

    int exponent = ((uf & expMask) >> 23);
    unsigned int fraction = uf & fracMask;                  // only fraction
    unsigned int fractionReverseMasked = uf & ~fracMask;    // uf except fraction
    unsigned int msb = uf & msbMask;

    unsigned int lsb = fraction & 0x1;
    unsigned int shiftedFraction = fraction >> 1;
    unsigned int roundedFraction = shiftedFraction;

    if ((uf & zeroMask) == 0)
        return uf;

    if (exponent == 255)
        return uf;

    if (lsb)
        roundedFraction = roundedFraction + (shiftedFraction & 0x1);

    if (exponent == 0) {            // denormalized number. shift fraction
        if (fraction == fracMask)   // all ones
            return fractionReverseMasked | 0x00400000u;
        else
            return fractionReverseMasked | roundedFraction;
    } else if (exponent == 1) {
        return msb | roundedFraction | 0x00400000u;
    } else {
        return msb | ((exponent - 1) << 23) | fraction;
    }
}
