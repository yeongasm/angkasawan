#pragma once
#ifndef LEARNVK_RENDERER_API_RENDERER_FLAG_BITS_H
#define LEARNVK_RENDERER_API_RENDERER_FLAG_BITS_H

#include "Library/Templates/Types.h"

enum BuildStatus
{
	Build_Status_Failed,
	Build_Status_Pending,
	Build_Status_Building,
	Build_Status_Compiled
};

enum ESurfaceOffset : size_t
{
	Surface_Offset_X	= 0x00,
	Surface_Offset_Y	= 0x01,
	Surface_Offset_Max	= 0x02
};

enum ESurfaceExtent : size_t
{
	Surface_Extent_Width	= 0x00,
	Surface_Extent_Height	= 0x01,
	Surface_Extent_Max		= 0x02
};

enum ColorChannel : size_t
{
	Color_Channel_Red	= 0x00,
	Color_Channel_Green = 0x01,
	Color_Channel_Blue	= 0x02,
	Color_Channel_Alpha = 0x03,
	Color_Channel_Max	= 0x04
};

enum RenderPassType
{
	RenderPass_Type_Graphics
};

enum RenderPassState
{
	RenderPass_State_New,
	RenderPass_State_Built
};

enum CommandBufferLevel : uint32
{
	CommandBuffer_Level_Primary		= 0x00,
	CommandBuffer_Level_Secondary	= 0x01
};

enum ShaderType : uint32
{
	Shader_Type_Unspecified = 0xFF,	
	Shader_Type_Vertex		= 0x00,
	Shader_Type_Fragment	= 0x01,
	Shader_Type_Max			= 0x02
};

enum TopologyType : uint32
{
	Topology_Type_Point			 = 0x00,
	Topology_Type_Line			 = 0x01,
	Topology_Type_Line_List		 = 0x02,
	Topology_Type_Triangle		 = 0x03,
	Topology_Type_Triangle_Strip = 0x04,
	Topology_Type_Triangle_Fan	 = 0x05,
	Topology_Type_Max			 = 0x06 // DO NOT USE!
};

enum PolygonMode : uint32
{
	Polygon_Mode_Fill	= 0x00,
	Polygon_Mode_Line	= 0x01,
	Polygon_Mode_Point	= 0x02,
	Polygon_Mode_Max	= 0x03 // DO NOT USE!
};

enum FrontFaceDir : uint32
{
	Front_Face_Counter_Clockwise = 0x00,
	Front_Face_Clockwise		 = 0x01,
	Front_Face_Max				= 0x02 // DO NOT USE!
};

enum CullingMode : uint32
{
	Culling_Mode_None	= 0x00,
	Culling_Mode_Front	= 0x01,
	Culling_Mode_Back	= 0x02,
	Culling_Mode_Front_And_Back = 0x03,
	Culling_Mode_Max = 0x04 // DO NOT USE!
};

enum SampleCount : uint32
{
	Sample_Count_1	= 0x00,
	Sample_Count_2	= 0x01,
	Sample_Count_4	= 0x02,
	Sample_Count_8	= 0x03,
	Sample_Count_16 = 0x04,
	Sample_Count_32 = 0x05,
	Sample_Count_64 = 0x06,
	Sample_Count_Max = 0x07 // DO NOT USE!
};

enum AttachmentType : uint32
{
	Attachment_Type_Color			= 0x00,
	Attachment_Type_Depth_Stencil	= 0x01,
	Attachment_Type_Max				= 0x02
};

enum AttachmentUsage : uint32
{
	Attachment_Usage_Input,
	Attachment_Usage_Output
};

enum AttachmentDimension : uint32
{
	Attachment_Dimension_1D = 0x00,
	Attachment_Dimension_2D = 0x01,
	Attachment_Dimension_3D = 0x02,
	Attachment_Dimension_Max = 0x03
};

// Depth testing
// Blending
// Front facing
// Front face orientation
// Stencil operations

#endif // !LEARNVK_RENDERER_API_RENDERER_FLAG_BITS_H