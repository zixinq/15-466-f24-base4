//
//  TextProgram.cpp
//  
//
//  Created by Zixin Qiu on 9/24/24.
//
#include "TextureProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< TextureProgram > texture_program(LoadTagEarly, []() -> TextureProgram const * {
    TextureProgram *ret = new TextureProgram();
    return ret;
});

TextureProgram::TextureProgram() {
    program = gl_compile_program(
        // Vertex shader
        "#version 330 core\n"
        "layout (location = 0) in vec4 vertex;\n"
        "out vec2 TexCoord;\n"
        "uniform mat4 CLIP_FROM_LOCAL;\n"
        "void main() {\n"
        "    gl_Position = CLIP_FROM_LOCAL * vec4(vertex.xy, 0.0, 1.0);\n"
        "    TexCoord = vertex.zw;\n"
        "}\n",

        // Fragment shader
        "#version 330 core\n"
        "in vec2 TexCoord;\n"
        "out vec4 fragColor;\n"
        "uniform sampler2D text;\n"
        "uniform vec3 textColor;\n"
        "void main() {\n"
        "    vec4 sampled = texture(text, TexCoord);\n"
        "    fragColor = vec4(textColor, 1.0) * sampled;\n"
        "}\n"
    );
    
    //look up the locations of vertex attributes:
    Position_vec4 = glGetAttribLocation(program, "vertex");
    TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");


    //look up the locations of uniforms:
    CLIP_FROM_LOCAL_mat4 = glGetUniformLocation(program, "CLIP_FROM_LOCAL");

    GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");
    GLuint textColor_vec3 = glGetUniformLocation(program, "textColor");

    glUseProgram(program);

    glUniform1i(TEX_sampler2D, 0);
    
    glUniform3f(textColor_vec3, 1.0f, 1.0f, 1.0f);

    glUseProgram(0);
    
}

TextureProgram::~TextureProgram() {
    glDeleteProgram(program);
    program = 0;
                                
}

