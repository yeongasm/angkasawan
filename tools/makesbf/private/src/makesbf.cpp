#include "makesbf.hpp"

#include "core.cmdline/cmdline.hpp"

#include "meshify_job.hpp"

namespace makesbf
{
lib::map<std::filesystem::path, MakeSbf::JobType> const MakeSbf::EXTENSION_TASK_MAP{ 
	{ ".gltf",	MakeSbf::JobType::Gltf_To_Mesh }, 
	{ ".glb",	MakeSbf::JobType::Gltf_To_Mesh },
	{ ".ktx2",	MakeSbf::JobType::Ktx2_To_Image }
};

MakeSbf::MakeSbf([[maybe_unused]] int argc, char** argv) :
	m_baseDirectory{ std::filesystem::path{ argv[0] }.remove_filename() }
{}

auto MakeSbf::mise_en_place(int argc, char** argv) -> bool
{
	std::filesystem::path input;
	std::filesystem::path output;
	std::filesystem::path filename;

	core::cmdline::ProgramOptions po{ "usage: makesbf [options] -i <input> -o <output>" };
	core::cmdline::Option inputOpt{ po, "-i", "--input", "Path to file or directory.", input };
	core::cmdline::Option outputOpt{ po, "-o", "--output-dir", "Output dir of SBF file.", output };
	core::cmdline::Option outFilenameOpt{ po, "-F", "--filename", "Name of SBF file.", filename };

	if (!po.parse(core::cmdline::CommandLine{ argc, argv }) || input.empty())
	{
		return false;
	}

	output.remove_filename();

	if (!std::filesystem::exists(output))
	{
		std::filesystem::create_directory(output);
	}

	std::filesystem::path const extension = input.extension();

	if ((input.has_filename() && input.stem() == L"*") || 
		std::filesystem::is_directory(input))
	{
		input.remove_filename();
		
		for (auto const& dir : std::filesystem::directory_iterator{ input })
		{
			auto const& path = dir.path();
			auto const ext = path.extension();

			if (std::filesystem::is_directory(path) || (!extension.empty() && ext != extension) || !EXTENSION_TASK_MAP.contains(ext))
			{
				continue;
			}

			std::filesystem::path const outFilename = path.stem().append(".sbf");

			MakeSbfJobDescription jobDescription{
				.input = path,
				.jobType = EXTENSION_TASK_MAP.at(ext).value()->second
			};

			if (!output.empty())
			{
				jobDescription.output = output / outFilename;
			}
			else
			{
				jobDescription.output = input.parent_path() / outFilename;
			}

			m_jobDescription.push_back(std::move(jobDescription));
		}
	}
	else
	{
		if (auto exist = EXTENSION_TASK_MAP.at(extension); exist)
		{
			MakeSbfJobDescription jobDescription{
				.input = input,
				.jobType = EXTENSION_TASK_MAP.at(extension).value()->second
			};

			auto const outFilename = (filename.empty()) ? input.filename().replace_extension(".sbf").string() : filename.replace_extension(".sbf");

			if (!output.empty())
			{
				jobDescription.output = output / outFilename;
			}
			else
			{
				jobDescription.output = input.parent_path() / outFilename;
			}

			m_jobDescription.push_back(std::move(jobDescription));
		}
	}

	return true;
}

auto MakeSbf::cook() -> void
{
	/**
	* NOTE(afiq):
	* Once we have our coroutine-based task scheduler, use that.
	*/
	for (auto const& description : m_jobDescription)
	{
		switch (description.jobType)
		{
		case JobType::Gltf_To_Mesh:
			_translate_gltf_to_sbf(description);
			break;
		case JobType::Ktx2_To_Image:
		default:
			break;
		}
	}
}

auto MakeSbf::_translate_gltf_to_sbf(MakeSbfJobDescription const& description) -> void
{
	MeshifyJobInfo info{
		.input = description.input,
		.output = description.output,
		.attributes = render::VertexAttribute::Position | render::VertexAttribute::Normal | render::VertexAttribute::TexCoord
	};

	MeshifyJob job{ info };

	if (auto result = job.execute(); result)
	{
		fmt::print("[Error]{}\n", result.value());
	}
}

}