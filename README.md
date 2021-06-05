# DXRrenderer
## Summary
This is a simple DXR renderer. It currently supports the following functions:
  1. Basic DXR pipeline.
  2. A camera with fixed a fixed position and fov.
  3. A GUI with no actual functionality related to this renderer.
 
## Build
  * Windows 10 OS
  * Visual Studio 2019
  * dxcompiler.dll
  * dxil.dll

## Materials
The following materials are implemented:
  * Lambert  
  * Mirror
  * Glass
  * Plastic
  * Disney_BRDF

## Light source
The following light sources are implemented:
  * Point   
  * Spot
  * Distant   
  * Area
  * Triangle      

## Using The App
To move the camera, press the W/S/A/D/Q/E keys. The camera can also be rotated by left-clicking on the window and dragging the mouse.You can also hold down the right-clicking and move the mouse to adjust the field of view.

UI provides object material modification and light source information settings.The "SqrtNum" is the square root of the number of samples, so increasing this value increases the total rendering time.If you change a setting or move the camera or press the R key, the render will reset and start accumulating samples again. 
