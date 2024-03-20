#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H

/**
 * Reads in inArray and corresponding size, as well as outIdxArrays top two idx's and their
 * corresponding values in the inArray, which has the highest values.
 *
 * @input inArray array of values to find the top two maxs
 * @input inArraySize size of the inArray array in entries
 * @inout outIdxArray array holding the idxs of the top two values ([0] idx has the larger value in inArray array)
 * @inout outValArray array holding the top two values ([0] has the larger value)
 */
void topTwoIdx(uint64_t* inArray, uint64_t inArraySize, uint8_t* outIdxArray, uint64_t* outValArray){
	
	outValArray[0] = 0;
	outValArray[1] = 0;

	for (uint64_t i = 0; i < inArraySize; i++){
		if (inArray[i] > outValArray[0]){
			outValArray[1] = outValArray[0];
			outValArray[0] = inArray[i];
			outIdxArray[1] = outIdxArray[0];
			outIdxArray[0] = i;
		}
		else if (inArray[i] > outValArray[1]){
			outValArray[1] = inArray[i];
			outIdxArray[1] = i;
		}
	}
}

void dynamicInputString(char* defaultString, int maxStringLength, char* outputString){
	
	int userStringLength;
	
	printf("------ Customized Settings of Target String ------\n");
	printf("Please enter a string (in extended ASCII with a maximum of %d characters), \nor enter nothing to apply default settings. (default: %s)\n", maxStringLength, defaultString);
	printf("Part of input string that exceeds the maximum length will get chopped.\n(You can manually adjust #define MAX_STRING_LENGTH_FACTOR in the source code file.)\n");
	printf("Now provide your string: ");
	
	// Read the string using fgets and remove potential trailing newline.
	fgets(outputString, maxStringLength, stdin);
	outputString[strcspn(outputString, "\n")] = '\0';	
	userStringLength = strlen(outputString);
	
	if (userStringLength == 0) {
		printf("Using default string: %s\n", defaultString);
		strcpy(outputString, defaultString);
	} else {
		printf("Using customized string: %s\n", outputString);
	
	}

}
#endif