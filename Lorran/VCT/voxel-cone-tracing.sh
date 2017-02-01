#!/bin/bash

## COMPILE AND LINK DEFINITIONS
# No included: Source/Graphic/Texture2D.cpp
#INCLUDES="find Source/ -name "*.cpp" | xargs"
INCLUDES="Source/Scene/Scenes/MultipleObjectsScene.cpp Source/Scene/Scenes/CornellScene.cpp Source/Scene/Scenes/DragonScene.cpp Source/Scene/Scenes/GlassScene.cpp Source/Time/Time.cpp Source/Shape/Transform.cpp Source/Shape/Mesh.cpp Source/Shape/StandardShapes.cpp Source/Application.cpp Source/Utility/External/tiny_obj_loader.cpp Source/Utility/ObjLoader.cpp Source/Graphic/Graphics.cpp Source/Graphic/Material/Shader.cpp Source/Graphic/Material/Material.cpp Source/Graphic/Material/MaterialStore.cpp Source/Graphic/Texture3D.cpp Source/Graphic/Camera/OrthographicCamera.cpp Source/Graphic/Camera/Camera.cpp Source/Graphic/Camera/PerspectiveCamera.cpp Source/Graphic/Renderer/MeshRenderer.cpp Source/Graphic/FBO/FBO.cpp"
COMPILE="g++ -std=c++11 -g voxel-cone-tracing.cpp $INCLUDES -o voxel-cone-tracing -I Include/ -lGL -lGLU -lGLEW -lglfw3 -lX11 -lXxf86vm -lXrandr -lpthread -lXi -ldl -lXinerama -lXcursor"

#############################################################################################
## BUILDING STARTS HERE

## BUILD REGULAR VERSION
echo "Compiling regular version build..."
${COMPILE}

echo "Done"
