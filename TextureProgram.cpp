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
    /*
    program = gl_compile_program(
            // Vertex shader
            "#version 330 core\n"
            "uniform mat4 CLIP_FROM_LOCAL; \n"
            "layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>\n"
            "void main() {\n"
            "   gl_Position = CLIP_FROM_LOCAL * vertex;\n"  // Vertex positions are directly passed
            "}\n",
            
            // Fragment shader
            "#version 330 core\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "   fragColor = vec4(1.0, 0.0, 0.0, 1.0);  // Solid red color for now\n"
            "}\n"
        );
        
        // Check if shader program linked successfully
        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        } else {
            std::cout << "Shader program linked successfully." << std::endl;
        }

        // Get attribute locations
        Position_vec4 = glGetAttribLocation(program, "vertex");  // Getting vertex attribute location
    }

    TextureProgram::~TextureProgram() {
        glDeleteProgram(program);
    }

    //Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
    //program = gl_compile_program(
                               
        //vertex shader:
        "#version 330\n"
        "uniform mat4 CLIP_FROM_LOCAL;\n"
        "in vec4 vertex;\n"
        "in vec4 Position;\n"
        "in vec2 TexCoord;\n"
        "out vec2 texCoord;\n"
        "out vec4 color;\n"
        "void main() {\n"
        "   gl_Position = CLIP_FROM_LOCAL * vec4(vertex.xy,0.0,1.0);\n"
        "   texCoord = vertex.zw; \n"
        //"    gl_Position = CLIP_FROM_LOCAL * Position;\n"
        //"    texCoord = TexCoord;\n"
        "}\n"
    ,
        //fragment shader:
        "#version 330\n"
        "uniform sampler2D TEX;\n"
        "uniform vec3 textColor;\n"
        "in vec2 texCoord;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    vec4 sampled = vec4(1.0,1.0,1.0, texture(TEX, texCoord).r);\n"
        "    fragColor = vec4(textColor, 1.0)*sampled;\n"
       // "    float alpha = texture(TEX, texCoord).r;\n"
        //"    fragColor = vec4(textColor, alpha);\n"
        //"    fragColor = vec4(1, 0, 0, 1);\n"
        "}\n"
                                  
         "#version 330\n"
         "uniform mat4 CLIP_FROM_LOCAL;\n"
         "in vec4 Position;\n"
         //"in vec2 TexCoord;\n"
         "out vec2 texCoord;\n"
         "void main() {\n"
         "    gl_Position = CLIP_FROM_LOCAL * vec4(Position.xy,0.0,1.0);\n"
         "    texCoord = Position.zw;\n"
         "}\n"
     ,
         //fragment shader:
         "#version 330\n"
         "uniform sampler2D TEX;\n"
         "uniform vec3 textColor;\n"
         "in vec2 texCoord;\n"
         "out vec4 fragColor;\n"
         "void main() {\n"
         "    vec4 sampled = vec4(1.0,1.0,1.0, texture(TEX, texCoord).r);\n"
         "    fragColor = vec4(textColor, 1.0)*sampled;\n"
         "}\n"
                                  
        
    );*/
    program = gl_compile_program(
        // Vertex shader
        "#version 330 core\n"
        "layout (location = 0) in vec4 vertex;\n"  // vertex.xy for position, vertex.zw for texture coordinates
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
    
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    } else {
        std::cout << "Shader program linked successfully." << std::endl;
    }
    
    //look up the locations of vertex attributes:
    Position_vec4 = glGetAttribLocation(program, "vertex");
    TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");


    //look up the locations of uniforms:
    CLIP_FROM_LOCAL_mat4 = glGetUniformLocation(program, "CLIP_FROM_LOCAL");

    GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");
    GLuint textColor_vec3 = glGetUniformLocation(program, "textColor");

    //set TEX to always refer to texture binding zero:
    glUseProgram(program); //bind program -- glUniform* calls refer to this program now

    glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0
    
    glUniform3f(textColor_vec3, 1.0f, 1.0f, 1.0f);

    glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now'
    
}

TextureProgram::~TextureProgram() {
    glDeleteProgram(program);
    program = 0;
                                
}

