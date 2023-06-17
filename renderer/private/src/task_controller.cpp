#include "task_controller.h"

namespace gpu::task_controller
{

rhi::command::CommandBuffer* get_command_buffer(TaskController& controller)
{
	if (!controller.currentCommandBuffer)
	{
		size_t& nextIndex = controller.mainQueuePool.next;
		controller.currentCommandBuffer = &controller.mainQueuePool.commandBuffers[nextIndex++];
	}
	auto commandBuffer = controller.currentCommandBuffer;
	if (!commandBuffer->is_recording())
	{
		commandBuffer->reset();
		if (!commandBuffer->begin())
		{
			commandBuffer = nullptr;
		}
	}
	return commandBuffer;
}

bool task_bind_raster_pipeline_processor(TaskController& controller, graph::task::TaskItem const& task)
{
	auto data = static_cast<graph::task::BindRasterPipeline*>(task.data);

	auto commandBuffer = get_command_buffer(controller);

	if (!commandBuffer)
	{
		return false;
	}

	commandBuffer->bind_pipeline(*data->rasterPipeline);

	return true;
}

bool task_memory_barrier_processor(TaskController& controller, graph::task::TaskItem const& task)
{
	auto data = static_cast<graph::task::MemoryBarrier*>(task.data);

	auto commandBuffer = get_command_buffer(controller);

	if (!commandBuffer)
	{
		return false;
	}

	commandBuffer->pipeline_barrier(data->memoryBarrier);

	return true;
}

bool task_buffer_memory_barrier_processor(TaskController& controller, graph::task::TaskItem const& task)
{
	auto data = static_cast<graph::task::BufferMemoryBarrier*>(task.data);

	auto commandBuffer = get_command_buffer(controller);

	if (!commandBuffer)
	{
		return false;
	}

	commandBuffer->pipeline_barrier(*data->buffer, data->bufferMemoryBarrier);

	return true;
}

bool task_image_memory_barrier_processor(TaskController& controller, graph::task::TaskItem const& task)
{
	auto data = static_cast<graph::task::ImageMemoryBarrier*>(task.data);

	auto commandBuffer = get_command_buffer(controller);

	if (!commandBuffer)
	{
		return false;
	}

	commandBuffer->pipeline_barrier(*data->image, data->imageMemoryBarrier);

	return true;
}

bool task_draw_processor(TaskController& controller, graph::task::TaskItem const& task)
{
	auto data = static_cast<graph::task::Draw*>(task.data);

	auto commandBuffer = get_command_buffer(controller);

	if (!commandBuffer)
	{
		return false;
	}

	// TODO(afiq):
	// Implement draw.

	return true;
}

bool task_blit_image_processor(TaskController& controller, graph::task::TaskItem const& task)
{
	auto data = static_cast<graph::task::BlitImage*>(task.data);

	auto commandBuffer = get_command_buffer(controller);

	if (!commandBuffer)
	{
		return false;
	}

	commandBuffer->blit_image(*data->srcImage, *data->dstImage, data->blitInfo);

	return true;
}

bool task_blit_swapchain_processor(TaskController& controller, graph::task::TaskItem const& task)
{
	auto data = static_cast<graph::task::BlitSwapchain*>(task.data);

	auto commandBuffer = get_command_buffer(controller);

	if (!commandBuffer)
	{
		return false;
	}

	commandBuffer->blit_image(*data->image, *data->swapchain, data->blitInfo);

	return true;
}

bool task_set_viewport_processor(TaskController& controller, graph::task::TaskItem const& task)
{
	auto data = static_cast<graph::task::SetViewport*>(task.data);
	auto commandBuffer = get_command_buffer(controller);

	if (!commandBuffer)
	{
		return false;
	}

	commandBuffer->set_viewport(*data->viewport);

	return true;
}

bool task_set_scissor_processor(TaskController& controller, graph::task::TaskItem const& task)
{
	auto data = static_cast<graph::task::SetScissor*>(task.data);
	auto commandBuffer = get_command_buffer(controller);

	if (!commandBuffer)
	{
		return false;
	}

	commandBuffer->set_scissor(*data->rect);

	return true;
}

using TaskItemProcessorFunction = bool(*)(TaskController&, gpu::graph::task::TaskItem const&);
using pair = typename std::unordered_map<rhi::RHIObjTypeID::value_type const, TaskItemProcessorFunction>::value_type;

static  std::unordered_map<pair::first_type, pair::second_type> const TASK_ITEM_FUNC = {
		pair{ rhi::rhi_obj_type_id_v<graph::task::BindRasterPipeline>,	task_bind_raster_pipeline_processor },
		pair{ rhi::rhi_obj_type_id_v<graph::task::MemoryBarrier>,		task_memory_barrier_processor },
		pair{ rhi::rhi_obj_type_id_v<graph::task::BufferMemoryBarrier>,	task_buffer_memory_barrier_processor },
		pair{ rhi::rhi_obj_type_id_v<graph::task::ImageMemoryBarrier>,	task_image_memory_barrier_processor },
		pair{ rhi::rhi_obj_type_id_v<graph::task::Draw>,				task_draw_processor },
		pair{ rhi::rhi_obj_type_id_v<graph::task::BlitImage>,			task_blit_image_processor },
		pair{ rhi::rhi_obj_type_id_v<graph::task::BlitSwapchain>,		task_blit_swapchain_processor },
		pair{ rhi::rhi_obj_type_id_v<graph::task::SetViewport>,			task_set_viewport_processor },
		pair{ rhi::rhi_obj_type_id_v<graph::task::SetScissor>,			task_set_scissor_processor }
};

bool TaskController::initialize()
{
	// Create graphics queue.
	if (!device->create_command_pool(mainQueuePool.commandPool))
	{
		return false;
	}
	size_t allocatedCount = 0;
	// Allocate command buffers from the pool.
	for (rhi::command::CommandBuffer& commandBuffer : mainQueuePool.commandBuffers)
	{
		if (!mainQueuePool.commandPool.allocate_command_buffer(commandBuffer))
		{
			break;
		}
		++allocatedCount;
	}

	if (allocatedCount != mainQueuePool.commandBuffers.size())
	{
		mainQueuePool.commandPool.release_command_buffers();

		return false;
	}

	return true;
}

void TaskController::terminate()
{
	mainQueuePool.commandPool.release_command_buffers();
}

void TaskController::process_task_item(graph::task::TaskItem const& task)
{
	if (!TASK_ITEM_FUNC.contains(task.type.get()))
	{
		return;
	}

	auto taskProcessor = TASK_ITEM_FUNC.at(task.type.get());

	if (!taskProcessor(*this, task))
	{
		// Do something.
	}
}

void TaskController::submit_tasks()
{
	currentCommandBuffer->end();
}

//void CommandQueue::record_commands()
//{
//	// TODO(afiq):
//}

}