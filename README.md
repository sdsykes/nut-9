# Back to school

Some observations about this particular puzzle.

1. The width can expand by one square either side when each new row is generated.
2. Checking for glide is easier if each line is saved trimmed of blank values at the ends. The offset (number of blanks trimmed from the start) must be saved.
3. Can use the same test for blink, and in this case the offsets will be the same.
4. When there are less than 3 filled squares the result is always vanishing.
5. But as we are limited to 100 rows, we should perhaps not check for 2 or less filled squares during the last couple of iterations, so this doesn't seem like a particularly worthwhile optimisation as there is a test to do also.
6. When generating a new line, counting the number of marked squares in all five positions is a little easier to write a condition for. The condition comes out as make a new mark if there are 3 or 5 marks above, or the square above is blank and there are 2 marks.

## Solution

I offer a Swift solution, [wunder.swift](/sdsykes/nut-9/blob/master/wunder.swift), which comes in at 37 SLOC. It is really the simplest thing that I could come up with.

Run like so (on a mac with Xcode installed):

    swift wunder.swift

To go a bit faster, you can compile it before running it:

    swiftc wunder.swift
    ./wunder

The patterns are processed one by one. Each pattern is read in to a PatternString struct (which is really just a container for a string and methods), and padded with 101 blank (".") elements either side so that there is enough room for whatever is generated as we want each line to be the same width.

Processing takes the form of a recursive function which generates a new line on each call. The initial line and each generated line is saved in an dictionary with the blank elements trimmed off each end. These trimmed strings are the dictionary keys, and "offsets" are the values. The offset is how many blank values were trimmed from the beginning of the string.

Testing for vanishing is easy, the size of the trimmed string will be zero. Gliding and blinking can be checked by checking the dictionary for matches, and the answer will be blinking if the offset also matches, or gliding if not. If there is no match or vanishing after 99 generated lines, then "other" is the result.


## Alternative less elegant but extremely fast implementation

For comparison and to verify my results, I developed a solution in C, [wunder.c](/sdsykes/nut-9/blob/master/wunder.c). Build with make.

Although the swift solution is rather elegant, to get maximum speed you would want to go to C and use bit operations. I developed a solution which does this, you may find it in wunder.c. The algorithm is basically the same as the swift version, except that instead of using strings I use groups of 64 bit integers to store the pattern and generated lines, using the binary values 1 for filled and 0 for blank.

This version also caches while generating each new line (which it does in 12 bit blocks), so that if a particular 12 bit block has already been seen then the cached (8 bit) result will be used. In the end it doesn't affect the speed much, so you may regard this as an evil excercise in premature optimisation.

The solution runs in around 8mS on my machine, compared to about 350mS for the Swift version. On the down side it is rather harder to follow the code, and it leaks a little memory as it runs (well, I simply do not bother to handle freeing it as it's a tiny amount over the lifetime of the program).

    make
    ./wunderc patterns.txt

