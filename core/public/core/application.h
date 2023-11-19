#pragma once
#ifndef CORE_APPLICATION_H
#define CORE_APPLICATION_H

#include "engine_api.h"

namespace core
{

class CORE_API Application
{
public:
	Application() = default;
	virtual ~Application() {};

	virtual auto initialize() -> bool	= 0;
	virtual auto run() -> void			= 0;
	virtual auto terminate() -> void	= 0;
};

}

#endif // !CORE_APPLICATION_H
