#pragma once
#ifndef RENDERER_RENDERER_FRAME_GRAPH_H
#define RENDERER_RENDERER_FRAME_GRAPH_H

#include "resource_store.h"
#include "task_queue.h"

namespace gpu
{

class Renderer;

namespace graph
{

/**
* TODO(afiq):
* 1. [ ] Add enabling/disabling of render graphs/passes. (Follow what Activision does with their TaskGraph. A null resource is inherited instead).
* 2. [ ] Reference inputs and attachments via handles instead of string view.
* 3. [ ] Allow cross graph dependencies.
* 
* NOTE(afiq):
* disabling a pass requires that the output attachment of the disabled pass referenced in an active pass be substitued with some default texture.
*
* GOAL(afiq):
* In the end of the day, users should be able to describe the pipeline through an XML/JSON file.
*/

struct AttachmentInfo
{
	rhi::ImageInfo imageInfo;
	rhi::ColorBlendInfo blendInfo; // Ignored for depth and stencil attachment.
};

struct RenderPassAttachmentInfo
{
	std::string_view name;
	std::string_view dependsOn;			// If used as a color attachment and is dependent on the output of another pass.
	std::optional<AttachmentInfo> info;	// If the info optional does not contain a value, then it is an inherited attachment.
};

struct CreatePipelineInfo
{
	std::span<Handle<Shader>> shaders;
	std::optional<rhi::RasterPipelineInfo> rasterPipelineInfo = rhi::RasterPipelineInfo{};
};

struct RasterPassInfo
{
	ftl::HashStringView	name;
	rhi::Viewport viewport;
	rhi::Rect2D	scissor;
	CreatePipelineInfo pipelineInfo;
	/*std::optional<ftl::HashStringView>		inheritedPipelineName;*/ // Might want to include this in the future for uber shaders.
	ftl::Array<std::string_view> inputs;
	ftl::Array<RenderPassAttachmentInfo> colorAttachments;
	std::optional<RenderPassAttachmentInfo> depthAttachment;
	std::optional<RenderPassAttachmentInfo>	stencilAttachment;

	/**
	* TODO(afiq):
	* Find a more programmable solution than this!
	*/
	std::function<void(task::TaskInterface&)> execute;
};

struct RenderPassAttachment
{
	ftl::HashStringView	name;
	ftl::Ref<rhi::Image> image;
	rhi::ColorBlendInfo	blendInfo;	// Ignored for depth and stencil attachment.
	uint32 references;
};

/**
* NOTE(afiq):
* I would prefer that a Pass be an aggregate.
* An aggregate in C++ is an array or class that does not have:
*	1. A user defined constructor.
*	2. Private or protected non-static data member.
*	3. Base class.
*	4. Virtual functions.
*/
struct Pass final
{
	struct Attachment
	{
		RenderPassAttachment* attachment;
		ftl::HashStringView dependency;
	};

	using AttachmentMap = ftl::Map<ftl::HashStringView, Attachment>;
	
	enum class Type
	{
		None,
		Raster,
		Compute,
		Transfer
	} type;

	rhi::DeviceQueueType runsOn;
	ftl::HashStringView	name;
	ftl::Ref<RasterPipeline> rasterPipeline;
	rhi::Viewport viewport;							// Give public access to avoid getters and setters. This property should be mutable.
	rhi::Rect2D	scissor;							// Give public access to avoid getters and setters. This property should be mutable.
	AttachmentMap colorAttachments;					// Render target attachments.
	std::optional<Attachment> depthAttachment;
	std::optional<Attachment> stencilAttachment;
	AttachmentMap inputs;							// Render target attachments that are converted into sampled images in the shader.
	rhi::command::RenderMetadata metadata;

	bool operator==(const Pass& rhs) const;
	bool operator!=(const Pass& rhs) const;
};

struct RenderGraphStore;

/**
* Context here represents the details of the render graph.
*/
struct Context final
{
	using PassIndices = ftl::Map<ftl::HashStringView, size_t>;
	
	ftl::HashStringView	name;
	PassIndices indices;			// Indexes the pass and associates them to the name.
	ftl::Array<Pass> passes;		// Sortable.
	RenderGraphStore* store;
	bool execute;
	bool dirty;
	bool compiled;
};

class RenderGraph final
{
private:
	friend class gpu::Renderer;

	Context* m_context;

	RenderPassAttachment*	get_attachment				(std::string_view name);
	RenderPassAttachment*	register_attachment_internal(RenderPassAttachmentInfo const& info, std::string* debugOutput = nullptr);
	void					resolve_color_attachments	(std::span<RenderPassAttachmentInfo> colorAttachmentsInfos, Pass& pass, std::string* debugOutput);
	void					resolve_depth_attachment	(RenderPassAttachmentInfo const& depthAttachmentInfo, Pass& pass, std::string* debugOutput);
	void					resolve_stencil_attachment	(RenderPassAttachmentInfo const& stencilAttachmentInfo, Pass& pass, std::string* debugOutput);
	void					resolve_inputs				(std::span<std::string_view> inputs, Pass& pass, std::string* debugOutput);
	bool					does_pass_name_exist		(std::string_view name, std::string* debugOutput) const;
	RasterPipeline*			store_raster_pipeline		(std::span<Handle<Shader>> hshaders, rhi::RasterPipelineInfo const& pipelineInfo, std::string* debugOutput);

	/*void					record						(queue::CommandQueue& queue);*/

public:
	RenderGraph(Context& context);
	~RenderGraph() = default;

	RENDERER_API bool				add_pass			(RasterPassInfo&& info, std::string* debugOutput = nullptr);
	RENDERER_API bool				create_attachment	(std::string_view name, AttachmentInfo const& info, std::string* debugOutput = nullptr);
	RENDERER_API bool				compile				();
	RENDERER_API void				run					();
	RENDERER_API void				stop				();
	RENDERER_API bool				is_compiled			() const;
	RENDERER_API bool				is_dirty			() const;
	RENDERER_API std::string_view	get_name			();
};

struct RenderGraphStore final
{
	using GraphTable = ftl::Map<ftl::HashStringView, size_t>;
	using AttachmentStore = ftl::Map<ftl::HashStringView, RenderPassAttachment>;

	ftl::StringPool& stringPool;			// A reference to the Renderer's string pool.
	resource::ResourceStore& resourceStore; // A reference to the Renderer's resource store.
	AttachmentStore	attachments;			// Stores all attachments used by each render graph.
	GraphTable ids;
	ftl::Array<Context>	contexts;
	task::TaskQueue queue;

	Context* add_render_graph(std::string_view name);

};

}

using Graph = graph::RenderGraph;

}

#endif // !RENDERER_RENDERER_FRAME_GRAPH_H

