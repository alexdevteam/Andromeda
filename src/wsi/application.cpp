#include <andromeda/wsi/application.hpp>

#include <andromeda/components/hierarchy.hpp>
#include <andromeda/util/log.hpp>
#include <andromeda/util/version.hpp>

#include <phobos/core/vulkan_context.hpp>

#include <andromeda/assets/assets.hpp>
#include <andromeda/assets/texture.hpp>
#include <andromeda/components/static_mesh.hpp>
#include <andromeda/components/camera.hpp>
#include <andromeda/components/transform.hpp>
#include <andromeda/components/mesh_renderer.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_phobos.h>
#include <imgui/imgui_impl_mimas.h>

namespace andromeda::wsi {

static void load_imgui_fonts(ph::VulkanContext& ctx, vk::CommandPool command_pool) {
	vk::CommandBufferAllocateInfo buf_info;
	buf_info.commandBufferCount = 1;
	buf_info.commandPool = command_pool;
	buf_info.level = vk::CommandBufferLevel::ePrimary;
	vk::CommandBuffer command_buffer = ctx.device.allocateCommandBuffers(buf_info)[0];

	vk::CommandBufferBeginInfo begin_info = {};
	begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	command_buffer.begin(begin_info);

	ImGui_ImplPhobos_CreateFontsTexture(command_buffer);

	vk::SubmitInfo end_info = {};
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers = &command_buffer;
	command_buffer.end();

	ctx.graphics_queue.submit(end_info, nullptr);

	ctx.device.waitIdle();
	ImGui_ImplPhobos_DestroyFontUploadObjects();
	ctx.device.freeCommandBuffers(command_pool, command_buffer);
}

Application::Application(size_t width, size_t height, std::string_view title)
	: window(width, height, title) {

	ph::AppSettings settings;
#if ANDROMEDA_DEBUG
	settings.enable_validation_layers = true;
#else
	settings.enabe_validation_layers = false;
#endif
	constexpr Version v = ANDROMEDA_VERSION;
	settings.version = { v.major, v.minor, v.patch };
	context.vulkan = std::unique_ptr<ph::VulkanContext>(ph::create_vulkan_context(*window.handle(), &io::get_console_logger(), settings));
	context.world = std::make_unique<world::World>();

	// Initialize imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingWithShift = false;
	io.ConfigWindowsMoveFromTitleBarOnly = true;
	ImGui_ImplMimas_InitForVulkan(window.handle()->handle);
	ImGui_ImplPhobos_InitInfo init_info;
	init_info.context = context.vulkan.get();
	ImGui_ImplPhobos_Init(&init_info);
	io.Fonts->AddFontDefault();
	vk::CommandPoolCreateInfo command_pool_info;
	command_pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	vk::CommandPool command_pool = context.vulkan->device.createCommandPool(command_pool_info);
	load_imgui_fonts(*context.vulkan, command_pool);
	context.vulkan->device.destroyCommandPool(command_pool);

	renderer = std::make_unique<renderer::Renderer>(context);
}

Application::~Application() {
	context.vulkan->device.waitIdle();
	renderer.reset(nullptr);
	window.close();
	context.world.reset(nullptr);
	context.vulkan.reset(nullptr);
}

static constexpr float quad_verts[] = {
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f
};

static constexpr uint32_t quad_indices[] = {
	0, 1, 2, 3, 4, 5
};

void Application::run() {
	using namespace components;

	Handle<Texture> tex = assets::load<Texture>(context, "data/textures/blank.png");
	Handle<Material> mat = assets::take<Material>({ .diffuse = tex });
	ecs::entity_t ent = context.world->create_entity();
	auto& mesh = context.world->ecs().add_component<StaticMesh>(ent);
	auto& trans = context.world->ecs().get_component<Transform>(ent);
	auto& material = context.world->ecs().add_component<MeshRenderer>(ent);
	mesh.mesh = assets::load<Mesh>(context, "data/meshes/dragon.glb");
	trans.position.x = 5.0f;
	trans.position.y = 0.0f;
	material.material = mat;

	ecs::entity_t cam = context.world->create_entity();
	context.world->ecs().add_component<Camera>(cam);

	while (window.is_open()) {
		window.poll_events();

		ImGui_ImplMimas_NewFrame();
		ImGui::NewFrame();
		
		// Create ImGui dockspace
		ImGuiWindowFlags flags =
			ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Dockspace", nullptr, flags);
		ImGui::PopStyleVar(3);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
			ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
				ImGuiDockNodeFlags_None);
		}

		if (ImGui::Begin("Scene view", nullptr)) {
			auto size = ImGui::GetContentRegionAvail();
//			renderer->resize_attachments(size.x, size.y);
//			ImGui::Image(ImGui_ImplPhobos_GetTexture(renderer->scene_texture()), size);
		}
		ImGui::End();

		// End ImGui dockspace
		ImGui::End();

		renderer->render(context);
	}
}

}