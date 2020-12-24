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
	Surface_Offset_X	= 0,
	Surface_Offset_Y	= 1,
	Surface_Offset_Max	= 2
};

enum ESurfaceExtent : size_t
{
	Surface_Extent_Width	= 0,
	Surface_Extent_Height	= 1,
	Surface_Extent_Max		= 2
};

enum ColorChannel : size_t
{
	Color_Channel_Red	= 0,
	Color_Channel_Green = 1,
	Color_Channel_Blue	= 2,
	Color_Channel_Alpha = 3,
	Color_Channel_Max	= 4
};

enum RenderPassType
{
	RenderPass_Type_Graphics	= 0,
	RenderPass_Type_Compute		= 1,
	RenderPass_Type_Max			= 2
};

enum RenderPassOrder
{
	RenderPass_Order_First		= 0,
	RenderPass_Order_Last		= 1,
	RenderPass_Order_InBetween	= 2
};

enum RenderPassState
{
	RenderPass_State_New,
	RenderPass_State_Built
};

enum RenderPassFlagBits : uint32
{
	RenderPass_Bit_None					= 0,
	RenderPass_Bit_Color_Input			= 1,
	RenderPass_Bit_Color_Output			= 2,
	RenderPass_Bit_DepthStencil_Input	= 3,
	RenderPass_Bit_DepthStencil_Output	= 4,
	RenderPass_Bit_No_Color_Render		= 5,
	RenderPass_Bit_No_DepthStencil_Render = 6
};

enum CommandBufferLevel : uint32
{
	CommandBuffer_Level_Primary		= 0,
	CommandBuffer_Level_Secondary	= 1
};

enum ShaderAttribFormat : uint32
{
	Shader_Attrib_Type_Float	= 3,
	Shader_Attrib_Type_Vec2		= 4,
	Shader_Attrib_Type_vec3		= 5,
	Shader_Attrib_Type_Vec4		= 6
};

enum ShaderType : uint32
{
	Shader_Type_Unspecified =  0xFF,	
	Shader_Type_Vertex		=  0,
	Shader_Type_Fragment	=  1,
	Shader_Type_Max			=  2
};

enum TopologyType : uint32
{
	Topology_Type_Point			 = 0,
	Topology_Type_Line			 = 1,
	Topology_Type_Line_List		 = 2,
	Topology_Type_Triangle		 = 3,
	Topology_Type_Triangle_Strip = 4,
	Topology_Type_Triangle_Fan	 = 5,
	Topology_Type_Max			 = 6
};

enum PolygonMode : uint32
{
	Polygon_Mode_Fill	= 0,
	Polygon_Mode_Line	= 1,
	Polygon_Mode_Point	= 2,
	Polygon_Mode_Max	= 3
};

enum FrontFaceDir : uint32
{
	Front_Face_Counter_Clockwise = 0,
	Front_Face_Clockwise		 = 1,
	Front_Face_Max				 = 2
};

enum CullingMode : uint32
{
	Culling_Mode_None			= 0,
	Culling_Mode_Front			= 1,
	Culling_Mode_Back			= 2,
	Culling_Mode_Front_And_Back = 3,
	Culling_Mode_Max			= 4
};

enum SampleCount : uint32
{
	Sample_Count_1	 = 0,
	Sample_Count_2	 = 1,
	Sample_Count_4	 = 2,
	Sample_Count_8	 = 3,
	Sample_Count_16  = 4,
	Sample_Count_32  = 5,
	Sample_Count_64  = 6,
	Sample_Count_Max = 7 // DO NOT USE!
};

enum TextureUsage : uint32
{
	Texture_Usage_Color			= 0,
	Texture_Usage_Color_HDR		= 1,
	Texture_Usage_Depth_Stencil = 2,
	Texture_Usage_Max			= 3
};

enum TextureType : uint32
{
	Texture_Type_1D  = 0,
	Texture_Type_2D  = 1,
	Texture_Type_3D  = 2,
	Texture_Type_Max = 3
};

enum ImageUsageFlags : uint32
{
	Image_Usage_Transfer_Src	= 1,
	Image_Usage_Transfer_Dst	= 2,
	Image_Usage_Sampled			= 4,
	Image_Usage_Color_Attachment		 = 16,
	Image_Usage_Depth_Stencil_Attachment = 32
};

enum AttachmentType : uint32
{
	Attachment_Type_Input,
	Attachment_Type_Output
};

enum BufferType : uint32
{
	Buffer_Type_Vertex	= 0,
	Buffer_Type_Index	= 1,
	Buffer_Type_Max		= 2
};

// Depth testing
// Blending
// Front facing
// Front face orientation
// Stencil operations

#endif // !LEARNVK_RENDERER_API_RENDERER_FLAG_BITS_H