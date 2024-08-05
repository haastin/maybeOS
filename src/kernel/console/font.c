#include "font.h"
#include "string.h"
#include "VGA_driver.h"

static bool is_psf1_font(void *);


void set_font(Font_Name font_name, Font * font){

    //would prefer a hashmap here instead, we'll see
    switch(font_name){
        case(gr928b_8x16):
            initialize_font(font_name, font);
            break;
    }
}

bool isCharBit(unsigned char* char_bitmap, unsigned char bits_shifted, Font * font){

    //by shifting the glyph pointer we can have a font width or height larger than the bit size of the underlying platform
    unsigned char bytes_shifted = bits_shifted/8;
    char_bitmap += bytes_shifted;
    bits_shifted %= 8;

     //isolate the MSB of the bitmap since that is our current bit 
    unsigned char adjusted_bitmap = (*char_bitmap << bits_shifted);
    return (adjusted_bitmap & (1 << (font->width-1))) ? true : false;
}

static inline bool is_psf1_font(void * psf_font_file){
   
    if (((PSF1_FontHeader *) psf_font_file)->magic == PSF1_FONT_MAGIC){
        return true;
    }
    else if (((PSF2_FontHeader *) psf_font_file)->magic == PSF2_FONT_MAGIC){
        return false;
    }
    return false;
}

void initialize_font(Font_Name font_name, Font* font){

     switch(font_name){
        case(gr928b_8x16):
            bool psf1_font = is_psf1_font(_binary_gr928b_8x16_psfu_start);
            //THIS IS BUGGED, SEE PSF2 USE OF ADDRESSES BELOW TO FIX
            if(psf1_font){
                PSF1_FontHeader * psf1_font = (PSF1_FontHeader *) &_binary_gr928b_8x16_psfu_start;
                font->width = 8;
                font->bytesperglyph = psf1_font->bytesperglyph;
                font->height = psf1_font->bytesperglyph; //height determines num bytes since width is always 8 bits for psf1
                font->glyph = (unsigned char *) (_binary_gr928b_8x16_psfu_start + sizeof(PSF1_FontHeader));
                font->numglyph = ((0x1 & psf1_font->fontMode)+1)*256; //either 256 or 512 glyphs
                font->hasUnicodeTable = ((0x2 & psf1_font->fontMode) | (0x4 & psf1_font->fontMode));
            }
            else{
                PSF2_FontHeader * psf2_font = (PSF2_FontHeader *) &_binary_gr928b_8x16_psfu_start;
                memcpy(font, &(psf2_font->numglyph), 16);
                font->glyph = (unsigned char *) psf2_font + psf2_font->headersize;
                font->hasUnicodeTable = (0x1 & (psf2_font->flags));
            }
            break;
     }
}
