# 2dGraphics
2D Graphics Engine created for CS345

Everything in the apps, expected, include, and src folders were created as starter and test code by Mike Reed. Every file outside of these folders is code written and tested solely by me, Alex Niculescu.

Features:
- Creation of bitmap stored as array of pixels
- Color blending modes (Clear, Src, Dst, SrcOver, DstOver, SrcIn, DstIn, SrcOut, DstOut, SrcATop, DstATop, Xor)
- Fill the entire canvas with the specified color, using the specified blendmode.
- Draw rectangle
- Draw convex polygon
- Draw custom path to draw any type of shape (can be used to draw SVG files with prior translation)
  - Uses winding math so that shapes defined in opposite directions will create holes
- Matrices to translate, rotate, and scale each image to the client's need
- Shaders: Custom colors or "loads" that can be drawn inside shapes
  - Bitmap Shader to draw external images (e.g. png files)
  - Custom color shaders that can be defined by user
  - Color linear gradient shader
  - Color radial gradient shader
  - Bilinear Interpolation to blend images
- Draw linear strokes with differnent widths and end cap styles
-  Draw a mesh of triangles, with optional colors and/or texture-coordinates at each vertex
-  Draw a quad created by triangles, used to change the skew, and more easily control how the quad looks

Usage: In the 2dGraphics directory, run the following commands
    make -m image
    ./image

This will create all of the example images specified in the /tests folder. To create a custom image, open the GCanvas.cpp file and edit the GDrawSomething() function.

