#include "ConfigParser.h"
#if _WIN32
#include "Platform/OS/Win.h"
#endif
#include "Library/Stream/Ifstream.h"

bool ConfigFileParser::InitializeFromConfigFile(EngineCreationInfo& CreateInfo)
{
	using namespace rapidjson;

	astl::Ifstream configFile;
	Document document;

	configFile.Open(".cfg");
	size_t fileSize = configFile.Size() + 1;
	char* buf = reinterpret_cast<char*>(astl::IMemory::Malloc(sizeof(uint8) * fileSize));
	configFile.Read(buf, fileSize);
	buf[fileSize - 1] = '\0';
	document.Parse(buf);
	astl::IMemory::Free(buf);

	if (document.HasParseError())	{ return false; }
	if (!document.IsObject())		{ return false; }

	// Do all configurations here ...
	if (document.HasMember("ApplicationName"))
	{
		CreateInfo.AppName = document["ApplicationName"].GetString();
	}

	if (document.HasMember("Multithreaded"))
	{
		CreateInfo.Multithreaded = document["Multithreaded"].GetBool();
	}

	CreateInfo.StartPosX = 0;
	if (document.HasMember("WindowXPos"))
	{
		CreateInfo.StartPosX = document["WindowXPos"].GetInt();
	}

	CreateInfo.StartPosY = 0;
	if (document.HasMember("WindowYPos"))
	{
		CreateInfo.StartPosY = document["WindowYPos"].GetInt();
	}

	CreateInfo.Width = 1280;
	if (document.HasMember("WindowWidth"))
	{
		CreateInfo.Width = document["WindowWidth"].GetInt();
	}

	CreateInfo.Height = 960;
	if (document.HasMember("WindowHeight"))
	{
		CreateInfo.Height = document["WindowHeight"].GetInt();
	}

	CreateInfo.FullScreen = false;
	if (document.HasMember("FullScreen"))
	{
		CreateInfo.FullScreen = document["FullScreen"].GetBool();
	}

	if (document.HasMember("Game"))
	{
		CreateInfo.GameModule = document["Game"].GetString();
	}

	return true;
}
