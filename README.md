# Ray-Tracing-v3

My 3rd attemp at a ray-tracer. Real-time with tens of thousands of triangles

![image](https://github.com/mariofvelez/Ray-Tracing-v3/assets/32421774/df0369db-8ea1-4d7a-93f9-8d2644842fdd)

Diffuse + Specular lighting BRDF with roughness. 256 samples per pixel, 4 ray casts per path

![image](https://github.com/mariofvelez/Ray-Tracing-v3/assets/32421774/3a917ddf-b39f-4a5f-b20d-db487217e944)

Visualization of the BVH. Notice how there is a larger triangle density near the head leading to a higher BVH node density. Boxes are colored based on the axis they were split by.

![image](https://github.com/mariofvelez/Ray-Tracing-v3/assets/32421774/1cc5000f-1f63-495b-ae52-a6faf046f977)

Early demonstration of loading obj files

# Features
- Loading from OBJ and MTL files
- BVH acceleration
- Reflections
- Vertex normals and texturing

# What I Learned
- Ray-triangle intersection
- BVH construction and linearization on CPU and traversal on GPU
- loading OBJ and MTL files and storing them in a SSBO
- Barycentric coordinate system for interpolating normals and textures
- Reflectance models including diffuse and specular

# Things to Implement
- BVH construction on GPU for dynamic scenes
- PBR materials
- Emissive materials (lights)
- Multiple Importance Sampling
- Image-based materials
- Load scenes from USD or USDZ file
- Switch to compute shaders instead of fragment shader
- Use of RT cores with DirectX 12 (This will be the next project)
- Frame accumulation for path tracing

# Things to Fix
- [FIXED] Takes long to load application on my desktop and takes 3GB of memory to compile shader (???)
- [FIXED] Separate BVH building and linearization. This should fix the missing triangles problem
