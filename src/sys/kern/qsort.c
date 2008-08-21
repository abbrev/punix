/*
 * QSort/bsearch Copyright (C) 2000-2003 Zeljko Juric
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#define CALLBACK __attribute__((__stkparm__))
typedef CALLBACK short(*compare_t)(const void *elem1, const void* elem2);

void qsort(void *list asm("a0"), short nmemb asm("d0"), short size asm("d1"), compare_t cmp asm("a1"))
{
	unsigned short gap, byte_gap, i, j;                
	char *p, *a, *b;
	for (gap = ((unsigned short)nmemb)>>1; gap > 0; gap >>= 1) {
		byte_gap = gap * (unsigned short)size;
		for (i = byte_gap; i < ((unsigned short)nmemb) * (unsigned short)size; i += size)
			for (p = (char*)list + i - byte_gap; p >= (char*)list; p -= byte_gap) {
			a = p;
			b = p + byte_gap;
			if (cmp(a, b) <= 0)
				break;
			j = size - 1;
			do {
				char temp = *a;
				*a++ = *b;
				*b++ = temp;
			} while (j--);

			}
	}
}

void *bsearch(const void *key asm("a0"), const void *bptr asm("a1"), short n asm("d0"), short w asm("d1"), compare_t cmp asm("a2"))
{
	unsigned short left = 0, right = n - 1, index;
	short rcmp;
	void *rptr;
	do {
		index = (left + right)>>1;
		if ((rcmp = cmp(key, rptr = (char*)bptr + (long)index * (unsigned short)w)) > 0)
			left = index+1;
		else if (rcmp < 0)
			right = index - 1;
		else
			return rptr;
	} while (left <= right);
	return 0;
}
