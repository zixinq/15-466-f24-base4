//
//  text_render.cpp
//  
//
//  Created by Zixin Qiu on 9/24/24.
//

#include "text_render.hpp"
#include <iostream>

#define FONT_SIZE 36
#define MARGIN (FONT_SIZE * .5)

TextRenderer::TextRenderer(const std::string &fontfile, int font_size) {
    // Initialize FreeType and HarfBuzz
    if (FT_Init_FreeType(&ft_library)) {
        std::cerr << "Could not initialize FreeType\n";
        exit(1);
    }
    if (FT_New_Face(ft_library, fontfile.c_str(), 0, &ft_face)) {
        std::cerr << "Failed to load font\n";
        exit(1);
    }
    FT_Set_Char_Size(ft_face, font_size * 64, font_size * 64, 0, 0);

    hb_font = hb_ft_font_create(ft_face, nullptr);
    hb_buffer = hb_buffer_create();
}

TextRenderer::~TextRenderer() {
    // Clean up resources
    hb_buffer_destroy(hb_buffer);
    hb_font_destroy(hb_font);
    FT_Done_Face(ft_face);
    FT_Done_FreeType(ft_library);
    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);
}

void TextRenderer::render_text(const std::string &text) {
    // Set up HarfBuzz buffer
    hb_buffer_reset(hb_buffer);
    hb_buffer_add_utf8(hb_buffer, text.c_str(), -1, 0, -1);
    hb_buffer_guess_segment_properties(hb_buffer);

    // Shape the text
    hb_shape(hb_font, hb_buffer, nullptr, 0);

    unsigned int len = hb_buffer_get_length(hb_buffer);
    hb_glyph_info_t *info = hb_buffer_get_glyph_infos(hb_buffer, nullptr);
    hb_glyph_position_t *pos = hb_buffer_get_glyph_positions(hb_buffer, nullptr);

    /* Draw, using cairo. */
    double width = 2 * MARGIN;
    double height = 2 * MARGIN;
    for (unsigned int i = 0; i < len; i++) {
        width += pos[i].x_advance / 64.0;
        height += pos[i].y_advance / 64.0;
    }
    
    if (HB_DIRECTION_IS_HORIZONTAL (hb_buffer_get_direction(hb_buffer)))
      height += FONT_SIZE;
    else
      width  += FONT_SIZE;
    
    /* Set up cairo surface. */
    cairo_surface_t *cairo_surface;
    cairo_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                            ceil(width),
                            ceil(height));
    cairo_t *cr;
    cr = cairo_create (cairo_surface);
    cairo_set_source_rgba (cr, 1., 1., 1., 1.);
    cairo_paint (cr);
    cairo_set_source_rgba (cr, 0., 0., 0., 1.);
    cairo_translate (cr, MARGIN, MARGIN);

    
    /* Set up cairo font face. */
    cairo_font_face_t *cairo_face;
    cairo_face = cairo_ft_font_face_create_for_ft_face (ft_face, 0);
    cairo_set_font_face (cr, cairo_face);
    cairo_set_font_size (cr, FONT_SIZE);

    /* Set up baseline. */
    if (HB_DIRECTION_IS_HORIZONTAL (hb_buffer_get_direction(hb_buffer)))
    {
      cairo_font_extents_t font_extents;
      cairo_font_extents (cr, &font_extents);
      double baseline = (FONT_SIZE - font_extents.height) * .5 + font_extents.ascent;
      cairo_translate (cr, 0, baseline);
    }
    else
    {
      cairo_translate (cr, FONT_SIZE * .5, 0);
    }

    cairo_glyph_t *cairo_glyphs = cairo_glyph_allocate (len);
    double current_x = 0;
    double current_y = 0;
    for (unsigned int i = 0; i < len; i++)
    {
      cairo_glyphs[i].index = info[i].codepoint;
      cairo_glyphs[i].x = current_x + pos[i].x_offset / 64.;
      cairo_glyphs[i].y = -(current_y + pos[i].y_offset / 64.);
      current_x += pos[i].x_advance / 64.;
      current_y += pos[i].y_advance / 64.;
    }
    cairo_show_glyphs (cr, cairo_glyphs, len);
    cairo_glyph_free (cairo_glyphs);

    cairo_surface_write_to_png (cairo_surface, "out.png");

    cairo_font_face_destroy (cairo_face);
    cairo_destroy (cr);
    cairo_surface_destroy (cairo_surface);

    hb_buffer_destroy (hb_buffer);
    hb_font_destroy (hb_font);

    FT_Done_Face (ft_face);
    FT_Done_FreeType (ft_library);
}
