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

namespace andromeda::wsi {

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

	Handle<Texture> tex = assets::load<Texture>(context, "data/textures/test.png");
	Handle<Material> mat = assets::take<Material>({ .diffuse = tex });
	ecs::entity_t ent = context.world->create_entity();
	auto& mesh = context.world->ecs().add_component<StaticMesh>(ent);
	auto& trans = context.world->ecs().get_component<Transform>(ent);
	auto& material = context.world->ecs().add_component<MeshRenderer>(ent);
	mesh.mesh = context.request_mesh(quad_verts, sizeof(quad_verts) / sizeof(float), quad_indices, 6);
	trans.position.x = 5.0f;
	trans.position.y = 0.0f;
	trans.rotation.y = 90.0f;
	material.material = mat;


	ecs::entity_t cam = context.world->create_entity();
	context.world->ecs().add_component<Camera>(cam);

	while (window.is_open()) {
		window.poll_events();

		renderer->render(context);
	}
}

}