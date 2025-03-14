#ifndef MESHIFY_MESHIFY_HPP
#define MESHIFY_MESHIFY_HPP

#include <filesystem>
#include <queue>
#include "lib/string.hpp"
#include "lib/map.hpp"

namespace makesbf
{
enum class JobType
{
	Gltf_To_Mesh,
	Ktx2_To_Image
};

class MakeSbf
{
public:
	MakeSbf(int argc, char** argv);

	auto mise_en_place(int argc, char** argv) -> bool;
	auto cook() -> void;

	auto add_job(std::filesystem::path&& input, std::filesystem::path&& output, std::filesystem::path&& filename) -> void;
private:
	static const lib::map<std::filesystem::path, JobType> EXTENSION_TASK_MAP;

	struct MakeSbfJobDescription
	{
		std::filesystem::path input;
		std::filesystem::path output;
		JobType jobType;
	};

	std::queue<MakeSbfJobDescription> m_jobDescription;
	std::filesystem::path m_baseDirectory;

	auto _translate_gltf_to_sbf(MakeSbfJobDescription const& description) -> void;
	auto _translate_ktx2_to_sbf(MakeSbfJobDescription const& description) -> void;
};

}

#endif // !MESHIFY_MESHIFY_HPP