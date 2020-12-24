#pragma once
#ifndef LEARNVK_RENDERER_ASSETS_IMAGE_H
#define LEARNVK_RENDERER_ASSETS_IMAGE_H

#include "Library/Templates/Types.h"

struct Image
{
	uint8* Data;
	size_t Size;
	float32 Width;
	float32 Height;
};

#endif // !LEARNVK_RENDERER_ASSETS_IMAGE_H