#include <stdio.h>
#include <stdlib.h>
#include <print>

#include "../src/font.hpp"


int main()
{
    int font_height_px = 64;
    Font consola_font = create_font_at_size(std::string_view{"../data/fonts/CONSOLA.ttf"}, font_height_px); 
    int atlas_width = 1024;
    int atlas_height = 1024;
    auto font_texture_atlas = create_font_texture_atlas(consola_font, atlas_width, atlas_height);

    std::string word_to_write{"Hoi Niels"};

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, consola_font.ttf_data.data(), 0))
    {
        std::print("Failed to initialize font.\n");
        return -1;
    }
    auto bitmap_width = 512;
    auto bitmap_height = 512;
    std::vector<unsigned char> bitmap;
    bitmap.resize(bitmap_width * bitmap_height);

    int x = 0;
    int idx = 0;
    for (char character: word_to_write)
    {
        /* how wide is this character */
        int ax;
        int lsb;
        stbtt_GetCodepointHMetrics(&info, character, &ax, &lsb);
        /* (Note that each Codepoint call has an alternative Glyph version which caches the work required to lookup the character character.) */

        /* get bounding box for character (may be offset to account for chars that dip above or below the line) */
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&info, character, consola_font.scale, consola_font.scale, &c_x1, &c_y1, &c_x2, &c_y2);
        
        /* compute y (different characters have different heights) */
        int y = consola_font.ascent + c_y1;
        
        /* render character (stride and offset is important here) */
        int byteOffset = x + roundf(lsb * consola_font.scale) + (y * bitmap_width);
        stbtt_MakeCodepointBitmap(&info, bitmap.data() + byteOffset, c_x2 - c_x1, c_y2 - c_y1, bitmap_width, consola_font.scale, consola_font.scale, character);

        /* advance x */
        x += roundf(ax * consola_font.scale);
        
        /* add kerning */
        int kern;
        // we need the next code point, so advance.
        kern = stbtt_GetCodepointKernAdvance(&info, character, word_to_write[idx + 1]);
        x += roundf(kern * consola_font.scale);

        idx += 1;
    }
    
    /* save out a 1 channel image */
    stbi_write_png("out.png", bitmap_width, bitmap_height, 1, bitmap.data(), bitmap_width);

}


//     /*
//      Note that this example writes each character directly into the target image buffer.
//      The "right thing" to do for fonts that have overlapping characters is
//      MakeCodepointBitmap to a temporary buffer and then alpha blend that onto the target image.
//      See the stb_truetype.h header for more info.
//     */
    
//     free(fontBuffer);
//     free(bitmap);
    
//     return 0;
// }