#include "RenderGraph.h"
#include <iostream>

RenderGraph::RenderGraph() = default;
RenderGraph::~RenderGraph() = default;

int RenderGraph::ImportTexture(const std::string& name) {
    if (textures.count(name)) return textures[name];
    int handle = nextTextureHandle++;
    textures[name] = handle;
    // TODO: Actually import texture resource
    return handle;
}

int RenderGraph::CreateTexture(const std::string& name) {
    int handle = nextTextureHandle++;
    textures[name] = handle;
    // TODO: Actually create texture resource
    return handle;
}

void RenderGraph::ReadTexture(int handle) {
    // TODO: Mark texture as read in this pass
    (void)handle;
}

void RenderGraph::WriteTexture(int handle) {
    // TODO: Mark texture as written in this pass
    (void)handle;
}

void RenderGraph::Execute() {
    for (auto& pass : passes) {
        if (pass->execute && pass->data) {
            pass->execute(*pass->data, *this);
        }
    }
}
