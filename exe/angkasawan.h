#pragma once
#ifndef EXE_ANGKASAWAN_H
#define EXE_ANGKASAWAN_H

#include "core/engine.h"

class Angkasawan
{
public:
	Angkasawan();
	~Angkasawan();

	bool start(int argc, char** argv);
	void run();
	void stop();
private:
	core::Engine mEngine;
};

#endif // !EXE_ANGKASAWAN_H
