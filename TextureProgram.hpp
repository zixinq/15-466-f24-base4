//
//  TextureProgram.hpp
//
//
//  Created by Zixin Qiu on 9/24/24.
//

 #ifndef TextureProgram_hpp
 #define TextureProgram_hpp
 
 #pragma once
 
 #include "GL.hpp"
 #include "Load.hpp"
 #include "Scene.hpp"
 

 //Shader program that draws textured mesh:
 struct TextureProgram {
 TextureProgram();
 ~TextureProgram();
 
 GLuint program = 0;
 
 //Attribute (per-vertex variable) locations:
 GLuint Position_vec4 = -1U;
 GLuint textColor_vec3 = -1U;
     

 GLuint TexCoord_vec2 = -1U;
 
 //Uniform (per-invocation variable) locations:
 GLuint CLIP_FROM_LOCAL_mat4 = -1U;
 GLuint textColor = -1U;
 //Textures:
 //TEXTURE0 - texture that is accessed by TexCoord
 };
 
 extern Load< TextureProgram > texture_program;
 #endif
 /*
 
 #ifndef TEXTURE_PROGRAM_HPP
 #define TEXTURE_PROGRAM_HPP
 
 #pragma once
 
 #include "GL.hpp"
 #include "Load.hpp"
 #include "Scene.hpp"
 
 
 
 // Basic Shader Program class to handle compilation and linking
 class TextureProgram {
 public:
 TextureProgram();
 ~TextureProgram();
 
 GLuint program;
 
 GLint Position_vec4;  // Attribute location for Position
 };
 extern Load< TextureProgram > texture_program;
 
 #endif
 
 */
