#pragma once
#ifndef LEARNVK_RENDERER_API_RENDERER_FLAG_BITS_H
#define LEARNVK_RENDERER_API_RENDERER_FLAG_BITS_H

#include "Library/Templates/Types.h"

enum EBuildStatus
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

enum EVertexInputRateType : uint32
{
	Vertex_Input_Rate_Vertex = 0,
	Vertex_Input_Rate_Instance = 1,
	Vertex_Input_Rate_Max = 2
};

enum EColorChannel : size_t
{
	Color_Channel_Red	= 0,
	Color_Channel_Green = 1,
	Color_Channel_Blue	= 2,
	Color_Channel_Alpha = 3,
	Color_Channel_Max	= 4
};

enum ERenderPassType : uint32
{
	RenderPass_Type_Graphics	= 0,
	RenderPass_Type_Compute		= 1,
	RenderPass_Type_Max			= 2
};

enum ERenderPassPassType : uint32
{
	RenderPass_Pass_Main	= 0,
	RenderPass_Pass_Sub		= 1
};

enum ERenderPassOrder : uint32
{
	RenderPass_Order_First		= 0,
	RenderPass_Order_Last		= 1,
	RenderPass_Order_InBetween	= 2
};

enum ERenderPassOrderValue : uint32
{
	RenderPass_Order_Value_First		= 1,
	RenderPass_Order_Value_Last			= 2,
	RenderPass_Order_Value_FirstLast	= 3,
	RenderPass_Order_Value_InBetween	= 4
};

enum ERenderPassState
{
	RenderPass_State_New,
	RenderPass_State_Built
};

enum ERenderPassFlagBits : uint32
{
	RenderPass_Bit_None						= 0,
	RenderPass_Bit_Color_Input				= 1,
	RenderPass_Bit_Color_Output				= 2,
	RenderPass_Bit_DepthStencil_Input		= 3,
	RenderPass_Bit_DepthStencil_Output		= 4,
	RenderPass_Bit_No_Color_Render			= 5,
	RenderPass_Bit_No_DepthStencil_Render	= 6,
	RenderPass_Bit_Depth_Input				= 7,
	RenderPass_Bit_Depth_Output				= 8,
	RenderPass_Bit_Stencil_Input			= 9,
	RenderPass_Bit_Stencil_Output			= 10
};

enum ECommandBufferLevel : uint32
{
	CommandBuffer_Level_Primary		= 0,
	CommandBuffer_Level_Secondary	= 1
};

enum EShaderAttribFormat : uint32
{
	Shader_Attrib_Type_Float	= 3,
	Shader_Attrib_Type_Vec2		= 4,
	Shader_Attrib_Type_Vec3		= 5,
	Shader_Attrib_Type_Vec4		= 6,
	Shader_Attrib_Type_Mat4		= 7
};

enum EShaderType : uint32
{
	Shader_Type_Vertex		= 0,
	Shader_Type_Fragment	= 1,
	Shader_Type_Geometry	= 2,
	Shader_Type_Compute		= 3,
	Shader_Type_Max			= 4
};

enum EShaderVarType : uint32
{
	Shader_Var_Type_Char		= 0,
	Shader_Var_Type_Int			= 1,
	Shader_Var_Type_Float		= 2,
	Shader_Var_Type_Vec2		= 3,
	Shader_Var_Type_Vec3		= 4,
	Shader_Var_Type_Vec4		= 5,
	Shader_Var_Type_Mat2		= 6,
	Shader_Var_Type_Mat3		= 7,
	Shader_Var_Type_Mat4		= 8,
	Shader_Var_Type_Sampler2D	= 9,
	Shader_Var_Type_Sampler3D	= 10
};

enum ETopologyType : uint32
{
	Topology_Type_Point			 = 0,
	Topology_Type_Line			 = 1,
	Topology_Type_Line_List		 = 2,
	Topology_Type_Triangle		 = 3,
	Topology_Type_Triangle_Strip = 4,
	Topology_Type_Triangle_Fan	 = 5,
	Topology_Type_Max			 = 6
};

enum EPolygonMode : uint32
{
	Polygon_Mode_Fill	= 0,
	Polygon_Mode_Line	= 1,
	Polygon_Mode_Point	= 2,
	Polygon_Mode_Max	= 3
};

enum EFrontFaceDir : uint32
{
	Front_Face_Counter_Clockwise = 0,
	Front_Face_Clockwise		 = 1,
	Front_Face_Max				 = 2
};

enum ECullingMode : uint32
{
	Culling_Mode_None			= 0,
	Culling_Mode_Front			= 1,
	Culling_Mode_Back			= 2,
	Culling_Mode_Front_And_Back = 3,
	Culling_Mode_Max			= 4
};

enum ESampleCount : uint32
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

enum ETextureUsage : uint32
{
	Texture_Usage_Color			= 0,
	Texture_Usage_Color_HDR		= 1,
	Texture_Usage_Depth_Stencil = 2,
	Texture_Usage_Max			= 3
};

enum ETextureType : uint32
{
	Texture_Type_1D  = 0,
	Texture_Type_2D  = 1,
	Texture_Type_3D  = 2,
	Texture_Type_Max = 3
};

enum EImageUsageFlags : uint32
{
	Image_Usage_Transfer_Src	= 0,
	Image_Usage_Transfer_Dst	= 1,
	Image_Usage_Sampled			= 2,
	Image_Usage_Color_Attachment		 = 3,
	Image_Usage_Depth_Stencil_Attachment = 4,
	Image_Usage_Max = 5
};

enum EAttachmentType : uint32
{
	Attachment_Type_Input,
	Attachment_Type_Output
};

enum EBufferType : uint32
{
	Buffer_Type_Vertex	= 0,
	Buffer_Type_Index	= 1,
	Buffer_Type_Uniform = 2,
	Buffer_Type_Transfer_Src = 3,
	Buffer_Type_Transfer_Dst = 4,
	Buffer_Type_Max		= 5
};

enum EBufferLocality : uint32
{
	Buffer_Locality_Cpu = 0,
	Buffer_Locality_Gpu = 1,
	Buffer_Locality_Cpu_To_Gpu = 2,
	Buffer_Locality_Max = 3
};

enum EDescriptorType : uint32
{
	Descriptor_Type_Uniform_Buffer			= 0,
	Descriptor_Type_Dynamic_Uniform_Buffer	= 1,
	Descriptor_Type_Sampled_Image			= 2,
	Descriptor_Type_Input_Attachment		= 3,
	Descriptor_Type_Max = 4
};

enum EDescriptorUsageType : uint32
{
	Descriptor_Usage_Global		= 0,
	Descriptor_Usage_Per_Pass	= 1,
	Descriptor_Usage_Per_Object = 2
};

// Depth testing
// Blending
// Front facing
// Front face orientation
// Stencil operations

#endif // !LEARNVK_RENDERER_API_RENDERER_FLAG_BITS_H