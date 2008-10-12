#include <stdio.h>

#define WORDSPERLINE 8

int main()
{
	int c0, c1;
	int n = 0;
	for (;;) {
		c0 = getchar();
		c1 = getchar();
		if (c1 == EOF) break;
		printf("0x%02x%02x,", c0, c1);
		if (!(++n % WORDSPERLINE)) {
			n = 0;
			putchar('\n');
		}
	}
	if (n % WORDSPERLINE) putchar('\n');
	return 0;
}
