#include <cstdlib>
#include "utils.h"

/**
* @brief Convert str to num 
*
* @param str the string containing the num
* @param num a reference to the number variable to hold the result
* 
* @return 0 on success, -1 on failure
*/
int strToNum(const char* str, int* num)
{
	char* end = nullptr;

	*num = strtol(str, &end, 10);

	if (*end == '\0')
	{
		return 0;
	}
	return -1;
}
