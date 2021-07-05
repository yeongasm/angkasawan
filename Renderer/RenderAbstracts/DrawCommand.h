#pragma once
#ifndef ANGKASA1_RENDERER_RENDER_ABSTRACTS_DRAW_COMMAND_H
#define ANGKASA1_RENDERER_RENDER_ABSTRACTS_DRAW_COMMAND_H

#include "RenderPlatform/API.h"
#include "Library/Containers/Array.h"
#include "Library/Containers/Node.h"
#include "Library/Math/Math.h"
#include "Primitives.h"

class IRenderSystem;

struct VertexInformation
{
  uint32 VertexOffset;
  uint32 NumVertices;
};

struct IndexInformation
{
  uint32 IndexOffset;
  uint32 NumIndices;
};

struct DrawSubmissionInfo
{
  Handle<SRenderPass> RenderPassHnd;
  Handle<SPipeline>   PipelineHnd;
  VertexInformation*  pVertexInformation;
  IndexInformation*   pIndexInformation;
  math::mat4*         pTransforms;
  void*               pConstants;
  size_t              ConstantTypeSize;
  uint32              DrawCount;
  uint32              TransformCount;
  uint32              ConstantsCount;
};

struct DrawCommand
{
  uint8  Constants[128];
  astl::Ref<SPipeline> pPipeline;
  uint32 NumVertices;
  uint32 NumIndices;
  uint32 VertexOffset;
  uint32 IndexOffset;
  uint32 InstanceOffset;
  uint32 InstanceCount;
  bool   HasPushConstants = false;
};

struct RenderStatistics
{
  uint32 NumObjectsSubmitted;
  uint32 NumVerticesSubmitted;
  uint32 NumIndicesSubmitted;
  uint32 NumTrianglesSubmitted;
};

#endif // !ANGKASA1_RENDERER_RENDER_ABSTRACTS_DRAW_COMMAND_H
