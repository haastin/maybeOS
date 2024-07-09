#include "font.h"
#include "string.h"
#include "VGA_driver.h"

static bool is_psf1_font(void *);


void set_font(Font_Name font_name){

    //would prefer a hashmap here instead, we'll see
    switch(font_name){
        case(gr928b_8x16):
            initialize_font(font_name, &curr_font);
            break;
    }
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

void initialize_font(Font_Name font_name, Font* curr_font){

     switch(font_name){
        case(gr928b_8x16):
            bool psf1_font = is_psf1_font(_binary_gr928b_8x16_psfu_start);
            //THIS IS BUGGED, SEE PSF2 USE OF ADDRESSES BELOW TO FIX
            if(psf1_font){
                PSF1_FontHeader * psf1_font = (PSF1_FontHeader *) &_binary_gr928b_8x16_psfu_start;
                curr_font->width = 8;
                curr_font->bytesperglyph = psf1_font->bytesperglyph;
                curr_font->height = psf1_font->bytesperglyph; //height determines num bytes since width is always 8 bits for psf1
                curr_font->glyph = (unsigned char *) (_binary_gr928b_8x16_psfu_start + sizeof(PSF1_FontHeader));
                curr_font->numglyph = ((0x1 & psf1_font->fontMode)+1)*256; //either 256 or 512 glyphs
                curr_font->hasUnicodeTable = ((0x2 & psf1_font->fontMode) | (0x4 & psf1_font->fontMode));
            }
            else{
                PSF2_FontHeader * psf2_font = (PSF2_FontHeader *) &_binary_gr928b_8x16_psfu_start;
                memcpy(curr_font, &(psf2_font->numglyph), 16);
                curr_font->glyph = (unsigned char *) psf2_font + psf2_font->headersize;
                curr_font->hasUnicodeTable = (0x1 & (psf2_font->flags));
            }
            break;
     }
}
