#include "gdt.h"

/*for the sake of convenience, we pass entire fields to build a segment descriptor and this will
parse them and properly divvy them up into their assigned places*/

void build_segment_descriptor(struct segment_descriptor * seg_desc, uint32_t baseaddress, uint32_t segmentlimit, uint8_t flag_lowbits, uint8_t flag_highbits){
   
   seg_desc->segmentlimit_lowbits = segmentlimit & 0x0000FFFF;
   seg_desc->seglimit_and_flags_highbits = ((segmentlimit >>16) & 0x000F) | flag_highbits;
   
   seg_desc->baseaddress_lowbits = baseaddress & 0x0000FFFF;
   seg_desc->baseaddress_middlebits = (baseaddress >> 16) & 0x00FF;
   seg_desc->baseaddress_highbits = (baseaddress >> 24) & 0xFF;

   seg_desc->flags_lowbits = flag_lowbits;
}