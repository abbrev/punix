#ifndef _GLYPH_H_
#define _GLYPH_H_

#define GLYPH_HEIGHT 6

struct glyph {
	unsigned short rows[GLYPH_HEIGHT];
};

/* 96 lower + 96 upper (excludes C0 + C1) */
struct glyphset {
	struct glyph lower[96];
	struct glyph upper[96];
};

void drawglyph(struct glyph *glyph, int row, int col);

#endif
