module;

#include "lib/string.h"
#include "lib/paged_array.h"
#include "rhi/device.h"

export module gpu.workgraph;

namespace gpu
{
/**
* This structure defines the initial setup for a node in the graph.
*/
export struct WorkInfo
{
	std::string_view name;
	rhi::DeviceQueue queue = rhi::DeviceQueue::Main;
};

struct WorkGraphContext;

struct WorkNode
{
	WorkInfo info;
	// We can deduce the type of work node this is from the pipeline supplied.
	// Can be null.
	rhi::Pipeline* pipeline;
};

export class Work
{
public:
	Work()  = default;
	~Work() = default;

private:
	friend class WorkGraphContext;

	Work(WorkGraphContext& workGraph, WorkNode& workNode);

	lib::ref<WorkGraphContext> m_graph;
	lib::ref<WorkNode> m_node;
};

export struct WorkGraph;

//export using work_node_handle = lib::opaque_handle<WorkNode, uint32, std::numeric_limits<uint32>::max(), WorkGraph>;

export struct WorkGraphInfo
{
	lib::string name;
};

struct WorkGraphContext
{
	using NodeContainer = lib::paged_array<WorkNode, 16>;
	using node_index	= typename NodeContainer::index;

	WorkGraphInfo info;
	NodeContainer nodes;

};

class WorkGraph
{
public:
	WorkGraph()  = default;
	~WorkGraph() = default;

	auto add_work(WorkInfo&& workInfo) -> std::optional<Work>;
	auto remove_work(Work& work) -> void;
	auto build() -> bool;
	auto dispatch() -> void;
protected:
	WorkGraphContext* m_graph;
};  

}