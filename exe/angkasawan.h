#pragma once
#ifndef EXE_ANGKASAWAN_H
#define EXE_ANGKASAWAN_H

#include "engine.h"

enum ErrorCode : int32
{
	Error_Code_Ok = 0,
	Error_Code_Bootstrap_Failed,
	Error_Code_Engine_Init_Failed,
	Error_Code_Load_Module_Failed
};

class Angkasawan
{
public:

	Angkasawan();
	~Angkasawan();

	bool start(int argc, char** argv);
	void run();
	void stop();

	ErrorCode get_last_error_code() const;

private:
	core::Engine m_engine;
	ErrorCode	 m_errorCode;

#ifdef PRODUCTION_BUILD
	void bootstrap_application_production(int argc, char** argv);
#else
	void bootstrap_application(int argc, char** argv);
#endif
};

#endif // !EXE_ANGKASAWAN_H
