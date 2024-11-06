#pragma once


#include <vector>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */

struct Font
{
	std::vector<unsigned char> ttf_data;
	int ascent;
	int descent;
	int line_gap;
	int line_height;
	float scale;
};

Font create_font_at_size(std::string_view path, int font_height)
{

    std::ifstream font_file(path.data(), std::ios::binary | std::ios::ate);
    if (!font_file)
    {
        std::print("Failed to open the font file.\n");
    }
    std::streamsize file_size = font_file.tellg();
    font_file.seekg(0, std::ios::beg);

    Font font{};
    font.ttf_data.resize(file_size);
    if (!font_file.read(reinterpret_cast<char*>(font.ttf_data.data()), file_size))
    {
        std::print("Failed to read the font file.\n");
    }

    // Initialize stb_truetype with the font buffer
    stbtt_fontinfo font_info{};
    if (!stbtt_InitFont(&font_info, font.ttf_data.data(), 0))
    {
        std::print("Failed to initialize font.\n");
    }

    // Success
    std::print("Font initialized successfully.\n");

    int line_height = font_height;

    /* calculate font scaling */
    float scale = stbtt_ScaleForPixelHeight(&font_info, line_height);
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
    
    ascent = roundf(ascent * scale);
    descent = roundf(descent * scale);

    // give this back.
    font.ascent = ascent;
    font.descent = descent;
    font.line_gap = line_gap;
    font.line_height = line_height;
    font.scale = scale;


    return font;
}


struct Font_Texture_Atlas
{
	Font* font = nullptr; // non-owning
	stbtt_packedchar character_info[96]; // ascii 32-127 mapping.
};

inline Font_Texture_Atlas create_font_texture_atlas(Font& font, int width, int height)
{
	Font_Texture_Atlas font_texture_atlas{};
	font_texture_atlas.font = &font;

	float font_size = font.line_height;
	unsigned char* ttf_buffer = font.ttf_data.data(); 
	int atlas_width = width;
	int atlas_height = height;
	std::vector<unsigned char> atlas_bitmap;
	atlas_bitmap.reserve(atlas_width * atlas_height);
	unsigned char* atlas_bitmap_ptr = atlas_bitmap.data();

	//Note(Sjors): we provide this now. We could also turn it into something else.
	// stbtt_packedchar character_info[96]; // Assuming 96 characters (ASCII 32-127) 
	stbtt_pack_context pack_context;
	stbtt_PackBegin(&pack_context, atlas_bitmap_ptr, atlas_width, atlas_height, 0, 1, NULL);
	stbtt_PackSetOversampling(&pack_context, 2, 2); // Optional oversampling for better quality
	stbtt_PackFontRange(&pack_context, ttf_buffer, 0, font_size, 32, 96, font_texture_atlas.character_info); // Packing ASCII 32-127
	stbtt_PackEnd(&pack_context);

	return font_texture_atlas;
}