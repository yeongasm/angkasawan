#pragma once
#include "gpu/common.hpp"
#ifndef SANDBOX_DEMO_APP_PIPELINE_DEFINITIONS_HPP
#define SANDBOX_DEMO_APP_PIPELINE_DEFINITIONS_HPP

#include "render/pipeline_definitions.hpp"

namespace sandbox
{
static inline constexpr render::RasterPipelineDefinition UBER_PIPELINE_DEFINITION = {
    .uri = "sponza uber pipeline",
    .shaderPaths = {
        .vertex = "data/demo/shaders/model.slang",
        .pixel = "data/demo/shaders/model.slang"
    },
    .info = {
        .name = "<pipeline.raster>:sponza uber pipeline",
        .colorAttachments = {
            gpu::ColorAttachment{ 
                .format = gpu::Format::B8G8R8A8_Srgb, 
                .blendInfo = { 
                    .enable = true,
                    .srcColorBlendFactor = gpu::BlendFactor::One,
                    .dstColorBlendFactor = gpu::BlendFactor::Zero,
                    .colorBlendOp = gpu::BlendOp::Add,
                    .srcAlphaBlendFactor = gpu::BlendFactor::One,
                    .dstAlphaBlendFactor = gpu::BlendFactor::Zero,
                    .alphaBlendOp = gpu::BlendOp::Add
                } 
            },
            gpu::ColorAttachment{
                .format = gpu::Format::R8G8_Unorm, 
                .blendInfo = { 
                    .enable = true,
                    .srcColorBlendFactor = gpu::BlendFactor::One,
                    .dstColorBlendFactor = gpu::BlendFactor::Zero,
                    .colorBlendOp = gpu::BlendOp::Add,
                    .srcAlphaBlendFactor = gpu::BlendFactor::One,
                    .dstAlphaBlendFactor = gpu::BlendFactor::Zero,
                    .alphaBlendOp = gpu::BlendOp::Add
                } 				
            },
            gpu::ColorAttachment{
                .format = gpu::Format::B8G8R8A8_Srgb, 
                .blendInfo = { 
                    .enable = true,
                    .srcColorBlendFactor = gpu::BlendFactor::One,
                    .dstColorBlendFactor = gpu::BlendFactor::Zero,
                    .colorBlendOp = gpu::BlendOp::Add,
                    .srcAlphaBlendFactor = gpu::BlendFactor::One,
                    .dstAlphaBlendFactor = gpu::BlendFactor::Zero,
                    .alphaBlendOp = gpu::BlendOp::Add
                } 				
            }   
        },
        .depthAttachmentFormat = gpu::Format::D32_Float,
        .rasterization = {
            .polygonalMode = gpu::PolygonMode::Fill,
            .cullMode = gpu::CullingMode::Back,
            .frontFace = gpu::FrontFace::Counter_Clockwise
        },
        .depthTest = {
            .depthTestCompareOp = gpu::CompareOp::Less,
            .minDepthBounds = 0.f,
            .maxDepthBounds = 1.f,
            .enableDepthBoundsTest = false,
            .enableDepthTest = true,
            .enableDepthWrite = true
        },
        .topology = gpu::TopologyType::Triangle,
        .pushConstantSize = sizeof(uintptr_t) * 3
    }
};
}

#endif // !SANDBOX_DEMO_APP_PIPELINE_DEFINITIONS_HPP