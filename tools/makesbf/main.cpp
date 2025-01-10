#include "makesbf.hpp"

auto main(int argc, char** argv) -> int
{
	makesbf::MakeSbf makeSbf{ argc, argv };

	if (makeSbf.mise_en_place(argc, argv))
	{
		makeSbf.cook();
	}

	return 0;
}