#pragma once
#include <vector>
#include "Data/Render/Types/PostProcessType.h"
#include "Data/Render/CPUData/RenderCommand.h"
#include "Data/Render/GPUData/RenderCommandGPU.h"

class DrawEngine;
class PostProcessRunner {
public:
    PostProcessRunner();

    void SetChain(const std::vector<PostProcessType>& chain);

    void Execute(DrawEngine* engine);
	RenderCommand& GetRenderCommand() { return renderCommand_; }
    void ChangeRenderCommand(const RenderCommand& command);
    void SetRenderCommand(DrawEngine* engine);

private:
    std::vector<PostProcessType> chain_;

    void RunPass(DrawEngine* drawEngine, PostProcessType type);
    RenderCommand renderCommand_{};
    RenderCommandGPU renderCommandForGPU_{};

};
