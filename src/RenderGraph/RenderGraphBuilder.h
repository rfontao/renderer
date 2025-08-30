#pragma once

class RenderGraphBuilder {
public:
    RenderGraphBuilder();
    ~RenderGraphBuilder();

    RenderGraphBuilder& AddPass();

    RenderGraphBuilder& AddColorInput();
    RenderGraphBuilder& AddColorOutput();
    RenderGraphBuilder& AddDepthInput();

    RenderGraphBuilder& AddTextureInput();

private:
    std::unique_ptr<RenderGraph> renderGraph;
    bool dirty;
};
