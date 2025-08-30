#pragma once

class RenderGraphPass {
    virtual void Execute(PassData) = 0;
};
