#include "Bit.h"

bool BinaryComparator(int binary, int target) {
	return (target & binary) == binary;
}