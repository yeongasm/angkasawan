#ifndef RENDERER_PUBLIC_RENDERER_TASK_CONTROLLER_H
#define RENDERER_PUBLIC_RENDERER_TASK_CONTROLLER_H

#include "render_graph.h"

namespace gpu
{

namespace task_controller
{

struct TaskController final
{
	/**
	* Each thread will have it's own command pool.
	*/
	struct CommandPool
	{
		using CommandBufferContainer = std::array<rhi::command::CommandBuffer, rhi::MAX_COMMAND_BUFFER_PER_POOL>;

		rhi::command::CommandPool commandPool;
		CommandBufferContainer commandBuffers;
		size_t next;
	};

	rhi::RenderDevice* device;
	CommandPool mainQueuePool;
	rhi::command::CommandBuffer* currentCommandBuffer;


	bool initialize	();
	void terminate	();
	void process_task_item(graph::task::TaskItem const& task);
	void submit_tasks();
};

}

}

#endif // !RENDERER_PUBLIC_RENDERER_TASK_CONTROLLER_H
