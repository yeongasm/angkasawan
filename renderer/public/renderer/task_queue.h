#ifndef RENDERER_RENDERER_COMMAND_DISPATCHER
#define RENDERER_RENDERER_COMMAND_DISPATCHER

#include "allocator/linear_allocator.h"
#include "containers/string.h"
#include "device.h"

namespace gpu
{

namespace graph
{
struct Pass;

namespace task
{

struct BindRasterPipeline
{
	rhi::RasterPipeline* rasterPipeline;
};

struct MemoryBarrier
{
	rhi::command::PipelineMemoryBarrier memoryBarrier;
};

struct BufferMemoryBarrier
{
	rhi::Buffer* buffer;
	rhi::command::BufferMemoryBarrier bufferMemoryBarrier;
};

struct ImageMemoryBarrier
{
	rhi::Image* image;
	rhi::command::ImageMemoryBarrier imageMemoryBarrier;
};

struct Draw
{
	rhi::command::RenderMetadata* metadata;
};

struct BlitImage
{
	rhi::Image* srcImage;
	rhi::Image* dstImage;
	rhi::command::ImageBlitInfo blitInfo;
};

struct BlitSwapchain
{
	rhi::Image* image;
	rhi::Swapchain* swapchain;
	rhi::command::ImageBlitInfo blitInfo;
};

struct SetViewport
{
	rhi::Viewport* viewport;
};

struct SetScissor
{
	rhi::Rect2D* rect;
};

struct TaskItem
{
	ftl::HashStringView owner;
	rhi::DeviceQueueType executionQueue;
	rhi::RHIObjTypeID type;
	void* data;

	std::string_view get_debug_name() const;
};

struct TaskQueue
{
	/**
	* A "Node" in a TaskQueue represents a render graph context. It is a collection of tasks from all the pass.
	*/
	struct Node
	{
		/**
		* Can't use a normal span because the array storing task items would resize and would invalidate the pointers.
		* Since they will not be moving around, it is fine to reference them by their index.
		*/
		struct index_span
		{
			size_t start = std::numeric_limits<size_t>::max();
			size_t count = 0;
		};

		ftl::LinearAllocator allocator;
		ftl::Array<TaskItem> tasks;			// A flat buffer to store all tasks from all pass within a render graph context.
		ftl::Array<index_span> taskGroups;	// The order of the elements inside of "taskGroups" coincide with the order that the passes in a render graph context will execute in.
		
		Node* previous;
		Node* next;

		Node();
		~Node();

		void flush();
		void release();
	};

	ftl::Map<ftl::HashStringView, Node> nodes;
	Node* start;
	Node* current;

	struct Iterator
	{
		Node*	data;
		size_t	group;
		size_t	counter;

		Iterator(Node* node);
		~Iterator();

		TaskItem&	operator*	() const;
		TaskItem*	operator->	() const;
		Iterator&	operator++	();
		bool		operator==	(Iterator const& rhs);
	};

	std::optional<Node&>	add_task_node	(ftl::HashStringView name);
	std::optional<Node&>	get_task_node	(ftl::HashStringView name);
	bool					remove_task_node(ftl::HashStringView name);
	Iterator				begin			();
	Iterator				end				();
};

/**
* TODO(afiq):
* Find a more programmable solution than this.
*/
class TaskInterface
{
private:
	graph::Pass& pass;
	TaskQueue::Node& taskNode;
	size_t groupIndex;
public:
	TaskInterface(graph::Pass& pass, TaskQueue::Node& node);

	TaskInterface&	bind_pipeline	();
	TaskInterface&	pipeline_barrier(rhi::command::PipelineMemoryBarrier&& barrier);
	TaskInterface&	pipeline_barrier(rhi::Buffer& buffer, rhi::command::BufferMemoryBarrier&& barrier);
	TaskInterface&	pipeline_barrier(rhi::Image& image, rhi::command::ImageMemoryBarrier&& barrier);
	TaskInterface&	blit_image		(rhi::Image& src, rhi::Image& dst, rhi::command::ImageBlitInfo&& info);
	TaskInterface&	blit_image		(rhi::Image& src, rhi::Swapchain& swapchain, rhi::command::ImageBlitInfo&& info);
	TaskInterface&	set_viewport	();
	TaskInterface&	set_scissor		();
};

}

}

}

#endif // !RENDERER_RENDERER_COMMAND_DISPATCHER
