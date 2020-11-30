#pragma once
#ifndef LEARNVK_EXECUTABLE_CONFIG_PARSER
#define LEARNVK_EXECUTABLE_CONFIG_PARSER

#include "rapidjson/document.h"
#include "Engine/Interface.h"

struct ConfigFileParser
{
	bool InitializeFromConfigFile(EngineCreationInfo& CreateInfo);
};

#endif // !LEARNVK_EXECUTABLE_CONFIG_PARSER