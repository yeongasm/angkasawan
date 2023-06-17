#include <simdjson.h>
#include "bootstrapper.h"
#include "containers/string.h"
#include "platform_interface/platform_interface.h"

COREBEGIN

Bootstrapper::Bootstrapper(literal_t path) :
	m_modules{}, m_moduleGraph{}, m_document{}, m_parser{}, m_json{}, m_good{}
{
	using namespace simdjson;
	auto errc = padded_string::load(path).get(m_json);
	if (errc == error_code::SUCCESS)
	{
		m_parser.iterate(m_json).get(m_document);
		m_good = true;
	}
}

Bootstrapper::~Bootstrapper() {}

bool Bootstrapper::good() const
{
	return m_good;
}

ErrorCode Bootstrapper::get_engine_init_info(EngineInitialization& init)
{
	// application name.
	ErrorCode status = get_application_name(init.applicationName);
	if (status != ErrorCode::Ok)
	{
		return status;
	}

	// system manager info.
	status = get_module_manager_info(init.moduleManager);
	if (status != ErrorCode::Ok)
	{
		return status;
	}

	// engine modules.
	status = get_list_of_modules(init.moduleManager);
	if (status != ErrorCode::Ok)
	{
		return status;
	}

	// These always returns ErrorCode::Ok, it's fine not to check.
	get_root_window_position(init.position);
	get_root_window_dimension(init.dimension);

	const bool skipFullscreenCheck = (init.dimension.width == -1 && init.dimension.height == -1);

	get_root_window_fullscreen(init.fullscreen, skipFullscreenCheck);
	get_root_window_file_drop(init.allowFileDrop);

	return ErrorCode::Ok;
}

ErrorCode Bootstrapper::translate_simdjson_error(simdjson::error_code errorCode) const
{
	ErrorCode errc = ErrorCode::Ok;

	if (errorCode == simdjson::TAPE_ERROR)
	{
		errc = ErrorCode::Bootstrap_File_Error;
	}

	if (errorCode == simdjson::NO_SUCH_FIELD)
	{
		errc = ErrorCode::Bootstrap_Missing_Configuration;
	}

	if (errorCode == simdjson::INCORRECT_TYPE)
	{
		errc = ErrorCode::Bootstrap_Invalid_Config_Value;
	}

	return errc;
}

ErrorCode Bootstrapper::get_application_name(ftl::UniString128& out) const
{
	// Reset document pointer.
	m_document.rewind();
	ErrorCode errc = ErrorCode::Ok;

	simdjson::ondemand::value param;
	simdjson::error_code error = m_document.find_field("application_name").get(param);

	if (error == simdjson::SUCCESS)
	{
		auto value = param.get_string();

		error = value.error();

		if (error == simdjson::SUCCESS)
		{
			literal_t str = value.value_unsafe().data();
			const size_t strlen = value.value_unsafe().length();

			size_t len = 0, index = 0;
			std::mbstate_t state{};
			wchar_t wc;

			while ((len = std::mbrtowc(&wc, str, strlen, &state)) > 0)
			{
				if (index >= strlen)
				{
					break;
				}
				out.add_ch(wc);
				str += len;
				++index;
			}
		}
	}

	errc = translate_simdjson_error(error);

	return errc;
}

ErrorCode Bootstrapper::get_root_window_position(Point& out) const
{
	// Reset document pointer.
	m_document.rewind();
	// Set 0, 0 as the default.
	out.x = out.y = 0;

	simdjson::ondemand::value param;
	simdjson::error_code error = m_document.find_field("root_window").get(param);

	if (error == simdjson::SUCCESS)
	{
		param.find_field("position").get(param);

		if (error == simdjson::SUCCESS)
		{
			int64 x, y;

			auto xpos = param.find_field("x");
			auto ypos = param.find_field("y");

			if (xpos.error() == simdjson::SUCCESS)
			{
				xpos.get(x);
				out.x = static_cast<int32>(x);
			}

			if (ypos.error() == simdjson::SUCCESS)
			{
				ypos.get(y);
				out.y = static_cast<int32>(y);
			}
		}
	}

	return ErrorCode::Ok;
}

ErrorCode Bootstrapper::get_root_window_dimension(Dimension& out) const
{
	// Reset document pointer.
	m_document.rewind();
	// Set -1, -1 as the default.
	out.width = out.height = -1;

	simdjson::ondemand::value param;
	simdjson::error_code error = m_document.find_field("root_window").get(param);

	if (error == simdjson::SUCCESS)
	{
		param.find_field("dimension").get(param);

		if (error == simdjson::SUCCESS)
		{
			int64 width, height;

			auto widthDim = param.find_field("width");
			auto heightDim = param.find_field("height");

			if (widthDim.error() == simdjson::SUCCESS)
			{
				widthDim.get(width);
				out.width = static_cast<int32>(width);
			}

			if (heightDim.error() == simdjson::SUCCESS)
			{
				heightDim.get(height);
				out.height = static_cast<int32>(height);
			}
		}
	}

	return ErrorCode::Ok;
}

ErrorCode Bootstrapper::get_root_window_fullscreen(bool& out, bool skip) const
{
	if (skip)
	{
		out = true;
		return ErrorCode::Ok;
	}
	out = false;

	m_document.rewind();

	simdjson::ondemand::value param;
	simdjson::error_code error = m_document.find_field("root_window").get(param);

	if (error == simdjson::SUCCESS)
	{
		auto value = param.find_field("fullscreen");

		if (value.error() == simdjson::SUCCESS)
		{
			value.get(out);
		}
	}

	return ErrorCode::Ok;
}

ErrorCode Bootstrapper::get_root_window_file_drop(bool& out) const
{
	out = false;
	m_document.rewind();
	simdjson::ondemand::value object;
	simdjson::error_code error = m_document.find_field("root_window").get(object);

	if (error == simdjson::SUCCESS)
	{
		object.find_field("file_drop").get(out);
	}

	return ErrorCode::Ok;
}

ErrorCode Bootstrapper::get_module_manager_info(ModuleManagerInfo& out) const
{
	m_document.rewind();
	simdjson::ondemand::object object;
	simdjson::error_code error = m_document.find_field("module_manager").get(object);

	if (error == simdjson::SUCCESS)
	{
		auto res0 = object.find_field("memory").get(out.memory);
		if (res0 != simdjson::SUCCESS)
		{
			return translate_simdjson_error(res0);
		}

		auto res1 = object.find_field("num_systems_hint").get(out.numSystemsHint);
		if (res1 != simdjson::SUCCESS)
		{
			return translate_simdjson_error(res1);
		}

		auto res2 = object.find_field("stack").get(out.stack);
		if (res2 != simdjson::SUCCESS)
		{
			return translate_simdjson_error(res2);
		}
	}

	return ErrorCode::Ok;
}

ErrorCode Bootstrapper::get_list_of_modules(ModuleManagerInfo& out)
{
	ErrorCode result = populate_module_dependency_graph(out);
	if (result == ErrorCode::Ok)
	{
		result = sort_module_dependency_graph(out.modules, out.numModules);
	}
	return result;
}

ErrorCode Bootstrapper::populate_module_dependency_graph(const ModuleManagerInfo& in)
{
	int32 order = 0;

	m_document.rewind();
	simdjson::ondemand::object modules;
	simdjson::error_code error = m_document.find_field("module_manager").find_field("modules").get(modules);

	if (error != simdjson::SUCCESS)
	{
		return ErrorCode::Bootstrap_Empty_Modules_List;
	}
	
	bool empty = false;
	modules.is_empty().get(empty);

	if (!empty)
	{
		// Add all modules into the graph first.
		for (auto module : modules)
		{
			ModuleType type{ ModuleType::None };
			size_t stack = 0ull;

			std::string_view moduleName;
			auto res0 = module.unescaped_key().get(moduleName);
			if (res0 != simdjson::SUCCESS)
			{
				return ErrorCode::Bootstrap_Missing_Configuration;
			}

			std::string_view moduleDll;

			auto object = module.value().get_object();
			auto res1 = object.find_field_unordered("dll").get(moduleDll);

			if (res0 != simdjson::SUCCESS ||
				res1 != simdjson::SUCCESS)
			{
				return ErrorCode::Bootstrap_Missing_Configuration;
			}

			object.find_field_unordered("stack").get(stack);

			std::string_view runMethod;
			auto res2 = object.find_field_unordered("run").get(runMethod);

			if (res2 != simdjson::NO_SUCH_FIELD)
			{
				if (runMethod == "sync")
				{
					type = ModuleType::Sync;
				}
				else if (runMethod == "async")
				{
					type = ModuleType::Async;
				}
				else
				{
					type = ModuleType::None;
				}
			}

			if (type == ModuleType::Sync)
			{
				m_moduleGraph.add_vertex(moduleName, moduleDll, stack, ++order, type);
			}
			else
			{
				m_moduleGraph.add_vertex(moduleName, moduleDll, stack, -1, type);
			}
		}
	}

	return ErrorCode::Ok;
}

ErrorCode Bootstrapper::sort_module_dependency_graph(ModuleInfo*& out, size_t& count)
{
	m_document.rewind();
	simdjson::ondemand::object modules;
	m_document.find_field("module_manager").find_field("modules").get(modules);

	bool empty = false;
	modules.is_empty().get(empty);

	if (!empty)
	{
		// Figure out module dependencies;
		for (auto module : modules)
		{
			std::string_view moduleName;
			module.unescaped_key().get(moduleName);

			auto object = module.value();
			simdjson::ondemand::array dependencies;

			if (object.find_field("dependencies").get(dependencies) == simdjson::SUCCESS)
			{
				for (auto name : dependencies)
				{
					std::string_view depName;
					name.get_string().get(depName);
					m_moduleGraph.add_edge(ModuleInfo{ .name = depName }, ModuleInfo{ .name = moduleName });
				}
			}
		}

		m_modules = m_moduleGraph.sort();

		out		= m_modules.data();
		count	= m_modules.size();
	}

	return m_moduleGraph.is_acyclic() ? ErrorCode::Ok : ErrorCode::Bootstrap_Cyclic_Module_Dependency_Detected;
}

COREEND