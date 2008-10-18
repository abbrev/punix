#ifndef _GLYPH_H_
#define _GLYPH_H_

#define GLYPH_HEIGHT 6

struct glyph {
	unsigned short rows[GLYPH_HEIGHT];
};

struct glyphset {
	struct glyph glyphs[96];
};

void drawglyph(struct glyph *glyph, int row, int col);

#endif
