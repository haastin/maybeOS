#ifndef __FONT_H__
#define __FONT_H__

#include <stdint.h>
#include <stdbool.h>

/*in the font file these symbols are defined around the data; the start symbol here
ends up being a symbol that is an array of this data*/
extern char _binary_gr928b_8x16_psfu_start[];
extern char _binary_gr928b_8x16_psfu_end[];

#define PSF1_FONT_MAGIC 0x0436
#define PSF2_FONT_MAGIC 0x864ab572

#define DEFAULT_FONT gr928b_8x16
#define DEFAULT_FONT_WIDTH
#define DEFAULT_FONT_HEIGHT


//PSF1 glyphs are always 8 bits wide
typedef struct {
    uint16_t magic; // Magic bytes for identification.
    uint8_t fontMode; // PSF font mode.
    uint8_t bytesperglyph; // PSF character size.
} PSF1_FontHeader;
 
typedef struct {
    uint32_t magic;         /* magic bytes to identify PSF */
    uint32_t version;       /* zero */
    uint32_t headersize;    /* offset of bitmaps in file, 32 */
    uint32_t flags;         /* 0 if there's no unicode table */
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
} PSF2_FontHeader;

typedef enum{
    gr928b_8x16 = 0
} Font_Name;

/*maybeOS has a single custom Font to get around two possible font types*/
typedef struct{
    uint32_t numglyph;      /* number of glyphs */
    uint32_t bytesperglyph; /* size of each glyph */
    uint32_t height;        /* height in pixels */
    uint32_t width;         /* width in pixels */
    unsigned char* glyph;
    bool hasUnicodeTable;
} Font;

void initialize_font(Font_Name, Font*);
void set_font(Font_Name font_name);

bool isCharBit(unsigned char *char_bitmap, unsigned char bits_shifted);

#endif /*__FONT_H__*/
