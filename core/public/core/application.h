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

	virtual bool initialize()	= 0;
	virtual void run()			= 0;
	virtual void terminate()	= 0;
};

}

#endif // !CORE_APPLICATION_H
