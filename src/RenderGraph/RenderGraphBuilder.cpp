//
// Created by Ricardo on 2025-08-26.
//

#include "RenderGraphBuilder.h"

RenderGraphBuilder::RenderGraphBuilder()
    : renderGraph(nullptr), dirty(true) {}

RenderGraphBuilder::~RenderGraphBuilder() = default;

RenderGraph* RenderGraphBuilder::Build() {
    if (dirty || !renderGraph) {
        // TODO: Add logic to build the actual render graph structure
        renderGraph = std::make_unique<RenderGraph>();
        dirty = false;
    }
    return renderGraph.get();
}

void RenderGraphBuilder::MarkDirty() {
    dirty = true;
}

RenderGraph* RenderGraphBuilder::GetRenderGraph() const {
    return renderGraph.get();
}

bool RenderGraphBuilder::IsDirty() const {
    return dirty;
}
