#include "render_graph.h"

namespace gpu::graph::task
{

std::string_view TaskItem::get_debug_name() const
{
	using pair = typename std::unordered_map<rhi::RHIObjTypeID::value_type const, literal_t>::value_type;

	static std::unordered_map<pair::first_type, literal_t> const TASK_DEBUG_NAME = {
		pair{ rhi::rhi_obj_type_id_v<BindRasterPipeline>,	"bind_raster_pipeline"	},
		pair{ rhi::rhi_obj_type_id_v<MemoryBarrier>,		"memory_barrier"		},
		pair{ rhi::rhi_obj_type_id_v<BufferMemoryBarrier>,	"buffer_memory_barrier" },
		pair{ rhi::rhi_obj_type_id_v<ImageMemoryBarrier>,	"image_memory_barrier"	},
		pair{ rhi::rhi_obj_type_id_v<Draw>,					"draw"					},
		pair{ rhi::rhi_obj_type_id_v<BlitImage>,			"blit_image"			},
		pair{ rhi::rhi_obj_type_id_v<BlitSwapchain>,		"blit_swapchain"		},
		pair{ rhi::rhi_obj_type_id_v<SetViewport>,			"set_viewport"			},
		pair{ rhi::rhi_obj_type_id_v<SetScissor>,			"set_scissor"			}
	};

	return std::string_view{ TASK_DEBUG_NAME.at(type.get()) };
}

TaskQueue::Node::Node() :
	allocator{ 64_KiB },
	tasks{},
	taskGroups{},
	previous{},
	next{}
{}

TaskQueue::Node::~Node()
{
	release();
}

void TaskQueue::Node::flush()
{
	taskGroups.empty();
	tasks.empty();
	allocator.flush();
}

void TaskQueue::Node::release()
{
	previous = next = nullptr;
	taskGroups.release();
	tasks.release();
	allocator.~LinearAllocator();
}

TaskQueue::Iterator::Iterator(TaskQueue::Node* data) :
	data{ data }, group{ 0ull }, counter{ 0ull }
{}

TaskQueue::Iterator::~Iterator()
{
	data = nullptr;
	group = 0ull;
	counter = 0ull;
}

TaskItem& TaskQueue::Iterator::operator*() const
{
	ASSERTION(data != nullptr);
	size_t const index = data->taskGroups[group].start + counter;
	return data->tasks[index];
}

TaskItem* TaskQueue::Iterator::operator->() const
{
	ASSERTION(data != nullptr);
	size_t const index = data->taskGroups[group].start + counter;
	return &data->tasks[index];
}

TaskQueue::Iterator& TaskQueue::Iterator::operator++()
{
	if (data)
	{
		if (counter >= data->taskGroups[group].count)
		{
			++group;
		}

		if (group >= data->taskGroups.size())
		{
			data = data->next;

			group = 0;
		}

		++counter;
	}

	return *this;
}

bool TaskQueue::Iterator::operator== (TaskQueue::Iterator const& rhs)
{
	return data != rhs.data && data != nullptr;
}

std::optional<TaskQueue::Node&> TaskQueue::add_task_node(ftl::HashStringView name)
{
	if (nodes.contains(name))
	{
		return std::nullopt;
	}

	auto& element = nodes.emplace(name, Node{});

	current = &element.second;
	
	if (start == nullptr)
	{
		start = &element.second;
	}
	else
	{
		Node* previous = current;
		current = &element.second;

		previous->next = current;
		current->previous = previous;
	}

	return std::make_optional<Node&>(element.second);
}

std::optional<TaskQueue::Node&> TaskQueue::get_task_node(ftl::HashStringView name)
{
	auto element = nodes.at(name);

	if (!element.has_value())
	{
		return std::nullopt;
	}

	return std::make_optional<Node&>(element.value()->second);
}

bool TaskQueue::remove_task_node(ftl::HashStringView name)
{
	auto element = nodes.at(name);

	if (!element.has_value())
	{
		return false;
	}

	Node& node = element.value()->second;

	if (node.previous)
	{
		node.previous->next = node.next;
	}

	if (node.next)
	{
		node.next->previous = node.previous;
	}

	nodes.erase(name);

	return true;
}

TaskQueue::Iterator TaskQueue::begin()
{
	return Iterator{ start };
}

TaskQueue::Iterator TaskQueue::end()
{
	return Iterator{ nullptr };
}

size_t add_task_group(TaskQueue::Node& node)
{
	return node.taskGroups.push(TaskQueue::Node::index_span{});
}

template <typename T, typename... Args>
bool push_task_into_node(TaskQueue::Node& node, size_t groupIndex, Pass& pass, Args&&... args)
{
	if (groupIndex >= node.taskGroup.size() ||
		node.allocator.remaining_space() < sizeof(T))
	{
		ASSERTION(groupIndex < node.taskGroup.size() && "Group index specified exceeds capacity of container!");
		ASSERTION(node.allocator.remaining_space() > sizeof(T) && "Allocator ran out of space. Increase size of allocator or swap to a more reliable one.");
		return false;
	}

	T* data = node.allocator.allocator_new<T>(std::forward<Args>(args)...);

	if (data)
	{
		size_t index = node.tasks.emplace(pass.name, pass.runsOn, rhi::rhi_obj_type_id_v<T>, static_cast<void*>(data));
		TaskQueue::Node::index_span& span = node.taskGroups[groupIndex];

		if (span.start == std::numeric_limits<size_t>::max())
		{
			span.start = index;
		}
		++span.count;
	}

	return data != nullptr;
}

TaskInterface::TaskInterface(graph::Pass& pass, TaskQueue::Node& node) :
	pass{ pass },
	taskNode{ node },
	groupIndex{}
{
	groupIndex = add_task_group(taskNode);
}

TaskInterface& TaskInterface::bind_pipeline()
{
	if (pass.type == gpu::graph::Pass::Type::Raster)
	{
		gpu::RasterPipeline* pipeline = pass.rasterPipeline.get();
		push_task_into_node<BindRasterPipeline>(taskNode, groupIndex, pass, pipeline);
	}
	return *this;
}

TaskInterface& TaskInterface::pipeline_barrier(rhi::command::PipelineMemoryBarrier&& barrier)
{
	push_task_into_node<MemoryBarrier>(taskNode, groupIndex, pass, std::move(barrier));
	return *this;
}

TaskInterface& TaskInterface::pipeline_barrier(rhi::Buffer& buffer, rhi::command::BufferMemoryBarrier&& barrier)
{
	push_task_into_node<BufferMemoryBarrier>(taskNode, groupIndex, pass, std::move(barrier));
	return *this;
}

TaskInterface& TaskInterface::pipeline_barrier(rhi::Image& image, rhi::command::ImageMemoryBarrier&& barrier)
{
	push_task_into_node<ImageMemoryBarrier>(taskNode, groupIndex, pass, std::move(barrier));
	return *this;
}

TaskInterface& TaskInterface::blit_image(rhi::Image& src, rhi::Image& dst, rhi::command::ImageBlitInfo&& info)
{
	push_task_into_node<BlitImage>(taskNode, groupIndex, pass, &src, &dst, std::move(info));
	return *this;
}

TaskInterface& TaskInterface::blit_image(rhi::Image& src, rhi::Swapchain& swapchain, rhi::command::ImageBlitInfo&& info)
{
	push_task_into_node<BlitSwapchain>(taskNode, groupIndex, pass, &src, &swapchain, std::move(info));
	return *this;
}

TaskInterface& TaskInterface::set_viewport()
{
	push_task_into_node<SetViewport>(taskNode, groupIndex, pass, &pass.viewport);
	return *this;
}

TaskInterface& TaskInterface::set_scissor()
{
	push_task_into_node<SetScissor>(taskNode, groupIndex, pass, &pass.scissor);
	return *this;
}

}