# FoveatedRenderingProject
Code used as part of my Cambridge undergraduate final year project.

Overview:
- *Main.cpp* - Entry point of the program, contains the main render loop.
- *shader.h, Shader.cpp* - Header file and code for a Shader class which handles reading, compiling and linking GLSL shaders, as well as functions for setting uniforms for said shaders.
- *Camera.h, FlyCamera.h* - Header-only abstract Camera class and a header-only FlyCamera implementation which ties mouse movement to viewing direction and WASD to camera movement (relative to viewing direction).
- *Scene.h, Scene.cpp* - Header and code for the Scene class, which handles loading the model using ASSIMP into a collection of Mesh objects, which are then stored. Also handles the drawing calls.
- *Mesh.h, Mesh.cpp* - Header and code for Mesh class, which handles creating all the OpenGL buffers required to draw a single mesh after being provided the data needed from the Scene object creating it.
- *vertexShader.gl, fragmentShader.gl* - The fragment and vertex shader used for the main rendering.
- *lightVertexShader.gl, lightFragmentShader.gl* - The shaders used for rendering the point light sources as fixed sized points - mainly used for debugging lighting.
- *blendingVertexShader.gl, blendingFragmentShader.gl* - The shaders used for rendering the single quad on which the eccentricity layer textures are blended on (which is done in blendingFragmentShader.gl).
