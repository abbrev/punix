#include <string.h>

#include "punix.h"
#include "process.h"
#include "param.h"
#include "proc.h"
#include "queue.h"
#include "inode.h"
#include "globals.h"
#include "exception.h"
#include "lcd.h"

#define NUM_PLANES 2 // we could support more than 2 but it would be flickery
#define NUM_PLANE_INDICES ((1<<(NUM_PLANES))-1)
//static const void *const grayplanes[NUM_PLANE_INDICES] = { 0x5c00, 0x6c00, 0x5c00 };

#define THIRD_SIZE 1280

#if 0
static void copythird(void *plane, int offset)
{
	// fast copy &plane[offset] to &LCD[offset]
	memmove(LCD_MEM + offset, plane + offset, THIRD_SIZE);
}
#else
extern void copythird(void *plane, int offset);
#endif

#define VSYNC (*(volatile char *)0x70001d)
/*
 * plane: 0=dark 1=light 2=repeated dark
 * third: 0=second 1280=first
 */
STARTUP(void updategray())
{
	if (!G.lcd.grayinitialized) return;
	int offset;
	int fs = (VSYNC ^ G.lcd.fs) & 0x80;

	if (fs) {
		G.lcd.fs ^= 0x80;
		// vsync
		// cycle to the next plane
		if (--G.lcd.planeindex < 0) {
			G.lcd.planeindex = NUM_PLANE_INDICES - 1;
		}
		void *p = G.lcd.planes[G.lcd.planeindex];
		if (p == G.lcd.currentplane) {
			// next plane is the same as the current plane
			// don't copy it to the screen
			return;
		}
		G.lcd.currentplane = p;
		offset = 2 * THIRD_SIZE;
		G.lcd.third = THIRD_SIZE;
	} else {
		// not vsync
		if (G.lcd.third < 0) {
			// and not currently copying a plane
			return;
		}
		// we still have to copy the first or second third of the screen
		// third = 0 or THIRD_SIZE
		offset = THIRD_SIZE-G.lcd.third;
		G.lcd.third -= THIRD_SIZE;
	}
	copythird(G.lcd.currentplane, offset);
}

STARTUP(void grayinit())
{
	size_t size = 3840*2;
	char *p0, *p1;
	p0 = memalloc(&size, 0); // allocate two contiguous buffers

	if (!p0) goto nomem;
	p1 = p0 + 3840;
	//memset(p0, 0, size);
	memmove(p0, LCD_MEM, 3840);
	//memmove(p1, LCD_MEM, 3840);
	memset(p1, 0, 3840);
	G.lcd.grayplanes[0] = p0;
	G.lcd.grayplanes[1] = p1;

	G.lcd.planes[0] = p0;
	G.lcd.planes[1] = p0;
	G.lcd.planes[2] = p1;
	G.lcd.third = -1;

#if 0
	/* gradient ramp */
	int i;
	for (i = 0; i < 3840; ++i) {
		p0[i] = (i/1920)&1 ? 0x00 : 0xff;
		p1[i] = (i/960)&1 ? 0x00 : 0xff;
	}
#endif

	G.lcd.grayinitialized = 1;
	return;

nomem:
	memfree(p0, 0);
}
