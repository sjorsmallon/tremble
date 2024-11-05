/* Created By: Justin Meiners (2013) */
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../inc/stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION 
#include "../inc/stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */



int main(int argc, const char * argv[])
{
    /* load font file */
    long size;
    unsigned char* fontBuffer;
    
    FILE* fontFile = fopen("data/CONSOLA.ttf", "rb");
    fseek(fontFile, 0, SEEK_END);
    size = ftell(fontFile); /* how long is the file ? */
    fseek(fontFile, 0, SEEK_SET); /* reset */
    fontBuffer = malloc(size);
    
    fread(fontBuffer, size, 1, fontFile);
    fclose(fontFile);

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, fontBuffer, 0))
    {
        printf("failed\n");
    }
    
    int bitmap_width = 512;
    int bitmap_height = 128; 
    int line_height = 64;

    /* create a bitmap for the phrase */
    unsigned char* bitmap = calloc(bitmap_width * bitmap_height, sizeof(unsigned char));
    
    /* calculate font scaling */
    float scale = stbtt_ScaleForPixelHeight(&info, line_height);

    char* word = "the quick brown fox";
    
    int x = 0;
       
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    
    ascent = roundf(ascent * scale);
    descent = roundf(descent * scale);
    
    int i;
    for (i = 0; i < strlen(word); ++i)
    {
        /* how wide is this character */
        int ax;
	    int lsb;
        stbtt_GetCodepointHMetrics(&info, word[i], &ax, &lsb);
        /* (Note that each Codepoint call has an alternative Glyph version which caches the work required to lookup the character word[i].) */

        /* get bounding box for character (may be offset to account for chars that dip above or below the line) */
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&info, word[i], scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
        
        /* compute y (different characters have different heights) */
        int y = ascent + c_y1;
        
        /* render character (stride and offset is important here) */
        int byteOffset = x + roundf(lsb * scale) + (y * bitmap_width);
        stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bitmap_width, scale, scale, word[i]);

        /* advance x */
        x += roundf(ax * scale);
        
        /* add kerning */
        int kern;
        kern = stbtt_GetCodepointKernAdvance(&info, word[i], word[i + 1]);
        x += roundf(kern * scale);
    }
    
    /* save out a 1 channel image */
    stbi_write_png("out.png", bitmap_width, bitmap_height, 1, bitmap, bitmap_width);
	
    /*
     Note that this example writes each character directly into the target image buffer.
     The "right thing" to do for fonts that have overlapping characters is
     MakeCodepointBitmap to a temporary buffer and then alpha blend that onto the target image.
     See the stb_truetype.h header for more info.
    */
    
    free(fontBuffer);
    free(bitmap);
    
    return 0;
}