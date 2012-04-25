#ifndef _GLYPH_H_
#define _GLYPH_H_

#define GLYPH_HEIGHT 6

struct glyph {
	unsigned char rows[2*GLYPH_HEIGHT];
};

struct glyphset {
	struct glyph glyphs[96];
};

void drawglyph(struct glyph const *glyph, int row, int col, void *lcd);
void drawglyphinv(struct glyph const *glyph, int row, int col, void *lcd);
void xorcursor(int row, int col, void *lcd);

#endif
