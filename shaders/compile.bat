%VK_SDK_PATH%/Bin/glslc.exe pbr.vert -o pbr.vert.spv
%VK_SDK_PATH%/Bin/glslc.exe pbr_bindless.frag -o pbr_bindless.frag.spv

%VK_SDK_PATH%/Bin/glslc.exe skybox.vert -o skybox.vert.spv
%VK_SDK_PATH%/Bin/glslc.exe skybox.frag -o skybox.frag.spv

%VK_SDK_PATH%/Bin/glslc.exe shadowmap.vert -o shadowmap.vert.spv
%VK_SDK_PATH%/Bin/glslc.exe shadowmap.frag -o shadowmap.frag.spv

%VK_SDK_PATH%/Bin/glslc.exe frustum.vert -o frustum.vert.spv
%VK_SDK_PATH%/Bin/glslc.exe frustum.frag -o frustum.frag.spv

%VK_SDK_PATH%/Bin/glslc.exe frustumCulling.comp -o frustumCulling.comp.spv
