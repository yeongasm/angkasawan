#include "angkasawan.h"

int main(int argc, char* argv[])
{
	Angkasawan app;
	if (app.start(argc, argv))
	{
		app.run();
		app.stop();
	}
	return 0;
}
