%VK_SDK_PATH%/Bin/glslc.exe pbr.vert -o pbr.vert.spv
%VK_SDK_PATH%/Bin/glslc.exe pbr.frag -o pbr.frag.spv

%VK_SDK_PATH%/Bin/glslc.exe skybox.vert -o skybox.vert.spv
%VK_SDK_PATH%/Bin/glslc.exe skybox.frag -o skybox.frag.spv

%VK_SDK_PATH%/Bin/glslc.exe pbr_bindless.frag -o pbr_bindless.frag.spv