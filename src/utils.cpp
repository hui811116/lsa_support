#include "utils.h"

int
count_bits(unsigned char b1, unsigned char b2)
{
	int count =0;
	unsigned char x = b1^b2;
	for(int i=0;i<8;++i){
		count = ( (x>>i) & 0x01)? count+1 : count;
	}
	return count;
}
