#pragma once
#ifndef SANDBOX_DEMO_APPLICATION_H
#define SANDBOX_DEMO_APPLICATION_H

namespace sandbox
{
struct Applet
{
	virtual ~Applet() = 0 {};

	virtual bool start() = 0;
	virtual void run() = 0;
	virtual void stop() = 0;
};
}

#endif // !SANDBOX_DEMO_APPLICATION_H
