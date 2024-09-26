//
//  text_render.hpp
//  
//
//  Created by Zixin Qiu on 9/24/24.
//

#ifndef text_render_hpp
#define text_render_hpp

#include <stdio.h>


#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include <cairo.h>
#include <cairo-ft.h>
#include <string>

class TextRenderer {
public:
    TextRenderer(const std::string &fontfile, int font_size);
    ~TextRenderer();

    void render_text(const std::string &text);

private:
    FT_Library ft_library;
    FT_Face ft_face;
    hb_font_t *hb_font;
    hb_buffer_t *hb_buffer;
    cairo_surface_t *cairo_surface;
    cairo_t *cr;
};

#endif /* text_render_hpp */
