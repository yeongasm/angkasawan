#pragma once
#ifndef RENDER_PIPELINE_DEFINITIONS_HPP
#define RENDER_PIPELINE_DEFINITIONS_HPP

#include <array>
#include <string_view>

#include "gpu/common.hpp"

namespace render
{
/*
* Can be defined in code but ideally should be defined in a text file somewhere
*/
struct RasterPipelineDefinition
{
    std::string_view uri;

    struct ShaderPaths
    {
        std::string_view vertex;
        std::string_view pixel;
        // std::string_view task;
        // std::string_view mesh;
        // std::string_view tesselationControl;
        // std::string_view tesselationEvaluation;
        
    } shaderPaths; // The pipeline cache will not look for the shaders in other directories than the one specified in these variables.

    struct Info
    {
        std::string_view name; // debug name
        std::array<gpu::ColorAttachment, 6> colorAttachments;
        uint32 numColorAttachments;
        gpu::Format depthAttachmentFormat;
	    gpu::Format stencilAttachmentFormat;
        gpu::RasterizationStateInfo rasterization;
        gpu::DepthTestInfo depthTest;
        gpu::TopologyType topology;
        uint32 pushConstantSize;

    } info;
};

/*
* Can be defined in code but ideally should be defined in a text file somewhere
*/
struct ComputePipelineDefinition
{
    std::string_view uri;

    struct ShaderPaths
    {
        std::string_view compute;

    } shaderPaths;

    struct Info
    {
        std::string_view name;
        uint32 pushConstantSize;
    } info;
};
}

#endif // !RENDER_PIPELINE_DEFINITIONS_HPP