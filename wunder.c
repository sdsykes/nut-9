#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef unsigned short CacheBlock;
#define CacheBlockSize 12
#define CacheSize 4096 // (1 << CacheBlockSize)
#define CacheResultSize 8
#define CacheMask 255

typedef struct CachedResult {
  bool known;
  CacheBlock block;
} CachedResult;
CachedResult cachedResults[CacheSize];

typedef unsigned long long LongBitBlock; // 64 bits on any sane platform
#define LongBitBlockSize sizeof(LongBitBlock)
#define NumbeOfBitsInLongBlock (LongBitBlockSize * 8)

typedef struct BitBlockLine {
  LongBitBlock *blocks;
  int numberOfBlocks;
  int numberOfSetBits;
  int firstSetBit;
} BitBlockLine;

#define MaxLine 256
#define Iterations 100

typedef enum PatternType {
  blink,
  glide,
  vanish,
  other
} PatternType;

// Pre-calculated for each 5 bit combination.
bool resultMap[32] = {0,0,0,1,0,0,0,1,0,1,1,1,0,1,1,0,
                      0,1,1,1,0,1,1,0,1,1,1,0,1,0,0,1};

BitBlockLine toBits(char *line) {
  int numberOfBlocks = (strlen(line) + Iterations * 2) / NumbeOfBitsInLongBlock + 1;
  LongBitBlock *blocks = calloc(numberOfBlocks, LongBitBlockSize);
  
  for (int i = 0; i < strlen(line); i++) {
    if (line[i] == '#') {
      blocks[(i + Iterations) / NumbeOfBitsInLongBlock] |= 1ULL << (i + Iterations) % NumbeOfBitsInLongBlock;
    }
  }
  
  BitBlockLine bitBlockLine = {blocks, numberOfBlocks};
  return bitBlockLine;
}

void printLine(BitBlockLine bitBlockLine) {
  for (int i = 0; i < bitBlockLine.numberOfBlocks; i++) {
    for (int j = 0; j < NumbeOfBitsInLongBlock; j++) {
      printf("%c", (bitBlockLine.blocks[i] >> j) & 1 ? '#' : '.');
    }
  }
  printf("\n");
}

// Process 12 bits, producing 8 bits of output padded by 2 bits.
CacheBlock process(CacheBlock source) {
  CacheBlock result = 0;
  for (int i = 0; i < CacheResultSize; i++) {
    CacheBlock mapIndex = source >> i & 31;
    if (resultMap[mapIndex]) result |= 1 << (i + 2);
  }
  return result;
}

// Memoize a 12 bit slices for performance.
CacheBlock processWithCache(CacheBlock sourceBits) {
  CacheBlock resultBits;
  
  if (cachedResults[sourceBits].known) {
    resultBits = cachedResults[sourceBits].block;
  } else {
    resultBits = process(sourceBits);
    cachedResults[sourceBits].block = resultBits;
    cachedResults[sourceBits].known = true;
  }
  return resultBits;
}

// Process a 64 bit block by splitting into 8 overlapping 12 bit slices, resulting in
// 8 bits of output per slice. Put these into the return value.
LongBitBlock processLongBlock(LongBitBlock block, int prev2Bits, int next2Bits) {
  LongBitBlock result = 0;
  int prev2, next2;

  for (int j = 0; j < NumbeOfBitsInLongBlock; j += CacheResultSize) {
    prev2 = j == 0 ? prev2Bits : (block >> (j - 2)) & 3;
    next2 = j + CacheResultSize < NumbeOfBitsInLongBlock ? (block >> (j + CacheResultSize)) & 3 : next2Bits;
    
    CacheBlock sourceBits = (block >> j) & CacheMask;
    sourceBits = (sourceBits << 2) | prev2;
    sourceBits |= next2 << (CacheBlockSize - 2);

    CacheBlock resultBits = processWithCache(sourceBits) >> 2;
    result |= (LongBitBlock)resultBits << j;
  }

  return result;
}

// Generate a new line from the given one using the rules. The line is
// processed a block at a time, taking care to provide the overlapping bits.
BitBlockLine generateLine(BitBlockLine bitBlockLine) {
  LongBitBlock *newBlocks = calloc(bitBlockLine.numberOfBlocks, LongBitBlockSize);
  for (int i = 0; i < bitBlockLine.numberOfBlocks; i++) {
    int prev2Bits = i == 0 ? 0 : bitBlockLine.blocks[i - 1] >> (NumbeOfBitsInLongBlock - 2);
    int next2Bits = i == bitBlockLine.numberOfBlocks - 1 ? 0 : bitBlockLine.blocks[i + 1] & 3;
    newBlocks[i] = processLongBlock(bitBlockLine.blocks[i], prev2Bits, next2Bits);
  }
  BitBlockLine newLine = {newBlocks, bitBlockLine.numberOfBlocks};
  return newLine;
}

int numberOfSetBits(LongBitBlock block) {
  LongBitBlock value = block;
  int c;
  for (c = 0; value; c++) value &= value - 1; // Kernighan's method
  return c;
}

// Shift the pattern in the line as far left as it can go. Set firstSetBit to how many
// shifts it took.
BitBlockLine shift(BitBlockLine bitBlockLine) {
  LongBitBlock *newBlocks = calloc(bitBlockLine.numberOfBlocks, LongBitBlockSize);
  for (int i = 0; i < bitBlockLine.numberOfBlocks; i++) {
    newBlocks[i] = bitBlockLine.blocks[i];
  }
  int firstSetBit = 0;
  while ((newBlocks[0] & 1) == 0) {
    firstSetBit++;
    for (int i = 0; i < bitBlockLine.numberOfBlocks; i++) {
      LongBitBlock newBlock = newBlocks[i] >> 1;
      if (i < bitBlockLine.numberOfBlocks - 1) {
        newBlock |= (newBlocks[i + 1] & 1) << (NumbeOfBitsInLongBlock - 1);
      }
      newBlocks[i] = newBlock;
    }
  }
  BitBlockLine newBitBlockLine = {newBlocks, bitBlockLine.numberOfBlocks, bitBlockLine.numberOfSetBits, firstSetBit};
  return newBitBlockLine;
}

bool compare(LongBitBlock *a, LongBitBlock *b, int numberOfBlocks) {
  for (int i = 0; i < numberOfBlocks; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

// Test a line against the history.
PatternType testLine(BitBlockLine bitBlockLine, BitBlockLine *bitBlockLineHistory, int iteration) {
  int totalSetBits = 0;
  for (int i = 0; i < bitBlockLine.numberOfBlocks; i++) {
    totalSetBits += numberOfSetBits(bitBlockLine.blocks[i]);
  }
  if (totalSetBits <= 2) return vanish; // Lines with two or less set bits don't survive.
  
  bitBlockLine.numberOfSetBits = totalSetBits;
  
  BitBlockLine shiftedLine = shift(bitBlockLine);
  bitBlockLineHistory[iteration] = shiftedLine;
  
  for (int i = iteration - 1; i >= 0; i--) {
    BitBlockLine historyLine = bitBlockLineHistory[i];
    if (historyLine.numberOfSetBits == totalSetBits) {
      if (compare(shiftedLine.blocks, historyLine.blocks, bitBlockLine.numberOfBlocks)) {
        if (historyLine.firstSetBit == shiftedLine.firstSetBit) return blink;
        return glide;
      }
    }
  }
  
  return other;
}

// Process one line from the input file, return its type.
PatternType processFileLine(char *line) {
  BitBlockLine bitBlockLineHistory[Iterations + 1];
  BitBlockLine blockLine = toBits(line);

  for (int i = 0; i < Iterations; i++) {
//    printLine(blockLine);
    PatternType type = testLine(blockLine, bitBlockLineHistory, i);
    if (type != other) return type;
    blockLine = generateLine(blockLine);
  }
  return other;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s patternfile\n", argv[0]);
    exit(1);
  }
  FILE *f = fopen(argv[1], "r");
  char line[MaxLine];
  while (fgets(line, MaxLine - 1, f)) {
    switch (processFileLine(line)) {
      case blink:
        printf("blinking\n");
        break;
      case glide:
        printf("gliding\n");
        break;
      case vanish:
        printf("vanishing\n");
        break;
      case other:
        printf("other\n");
        break;
    }
  }
}
