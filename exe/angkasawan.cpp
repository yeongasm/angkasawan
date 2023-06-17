#include "angkasawan.h"
#include "utility/bootstrapper.h"

Angkasawan::Angkasawan() :
	m_engine{}, m_errorCode{ Error_Code_Ok }
{}

Angkasawan::~Angkasawan() {}

bool Angkasawan::start([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
#ifdef PRODUCTION_BUILD
	bootstrap_application_production(argc, argv);
#else
	bootstrap_application(argc, argv);
#endif

	return m_errorCode == Error_Code_Ok;
}

void Angkasawan::run()
{
	m_engine.run();
}

void Angkasawan::stop()
{
	m_engine.terminate();
}

ErrorCode Angkasawan::get_last_error_code() const
{
	return m_errorCode;
}

#ifdef PRODUCTION_BUILD

void Angkasawan::bootstrap_application_production([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	// TODO(Afiq):
	// Implement production bootstrapping when the time is right.
}

#else

void Angkasawan::bootstrap_application([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
	m_errorCode = Error_Code_Bootstrap_Failed;

	core::EngineInitialization init{};
	core::Bootstrapper bootstrap{ ".bootstrap" };

	if (bootstrap.good() &&
		bootstrap.get_engine_init_info(init) == core::ErrorCode::Ok)
	{
		m_errorCode = Error_Code_Ok;
		m_engine.initialize(init);
	}
}

#endif