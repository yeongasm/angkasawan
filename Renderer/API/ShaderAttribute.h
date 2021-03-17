#pragma once
#ifndef LEARNVK_RENDERER_API_SHADER_ATTRIBUTE_H
#define LEARNVK_RENDERER_API_SHADER_ATTRIBUTE_H

#include "RendererFlagBits.h"
#include "Library/Templates/Templates.h"

struct ShaderAttrib
{
	//uint32 Binding;
	uint32 Location;
	uint32 Offset;
	EShaderAttribFormat Format;

	ShaderAttrib() :
		/*Binding(0), */Location(0), Offset(0), Format(Shader_Attrib_Type_Float)
	{};

	ShaderAttrib(/*uint32 InBinding, */uint32 InLocation, uint32 InOffset, EShaderAttribFormat InFormat) :
		/*Binding(InBinding), */Location(InLocation), Offset(InOffset), Format(InFormat)
	{};

	~ShaderAttrib() {};

	ShaderAttrib(const ShaderAttrib& Rhs) { *this = Rhs; };
	ShaderAttrib(ShaderAttrib&& Rhs) { *this = Move(Rhs); };

	ShaderAttrib& operator=(const ShaderAttrib& Rhs)
	{
		if (this != &Rhs)
		{
			//Binding = Rhs.Binding;
			Location = Rhs.Location;
			Offset = Rhs.Offset;
			Format = Rhs.Format;
		}
		return *this;
	};

	ShaderAttrib& operator=(ShaderAttrib&& Rhs)
	{
		if (this != &Rhs)
		{
			//Binding = Rhs.Binding;
			Location = Rhs.Location;
			Offset = Rhs.Offset;
			Format = Rhs.Format;

			new (&Rhs) ShaderAttrib();
		}
		return *this;
	};
};

#endif // !LEARNVK_RENDERER_API_SHADER_ATTRIBUTE_H