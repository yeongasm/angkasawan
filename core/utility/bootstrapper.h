#pragma once
#ifndef CORE_UTILITY_BOOTSTRAPPER_H
#define CORE_UTILITY_BOOTSTRAPPER_H

#include <simdjson.h>
#include "containers/string.h"
#include "containers/array.h"
#include "core_minimal.h"
#include "shared/engine_common.h"
#include "graphs/dag.h"

namespace core
{

class ENGINE_API Bootstrapper
{
public:
	Bootstrapper(literal_t path);
	~Bootstrapper();

	bool good() const;

	ErrorCode get_engine_init_info	(EngineInitialization& init);

private:
	using document		= simdjson::ondemand::document;
	using parser		= simdjson::ondemand::parser;
	using padded_string = simdjson::padded_string;

	using ModuleList	= ftl::Array<ModuleInfo>;
	using ModuleGraph	= ftl::Digraph<ModuleInfo>;
	
	ModuleList			m_modules;
	ModuleGraph			m_moduleGraph;

	mutable document	m_document;
	parser				m_parser;
	padded_string		m_json;
	bool				m_good;

	ErrorCode	translate_simdjson_error(simdjson::error_code errorCode) const;

	// Engine init util functions.
	ErrorCode	get_application_name		(ftl::UniString128& out)	const;
	ErrorCode	get_root_window_position	(Point& out)				const;
	ErrorCode	get_root_window_dimension	(Dimension& out)			const;
	ErrorCode	get_root_window_fullscreen	(bool& out, bool skip)		const;
	ErrorCode	get_root_window_file_drop	(bool& out)					const;
	ErrorCode	get_module_manager_info		(ModuleManagerInfo& out)	const;

	ErrorCode	get_list_of_modules					(ModuleManagerInfo& out);
	ErrorCode	populate_module_dependency_graph	(const ModuleManagerInfo& in);
	ErrorCode	sort_module_dependency_graph		(ModuleInfo*& out, size_t& count);
};

}

#endif // !CORE_UTILITY_BOOTSTRAPPER_H
