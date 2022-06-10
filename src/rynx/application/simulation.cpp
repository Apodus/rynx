
#include <rynx/application/simulation.hpp>
#include <rynx/filesystem/virtual_filesystem.hpp>
#include <rynx/tech/ecs.hpp>

rynx::application::simulation::simulation(rynx::scheduler::task_scheduler& scheduler) : m_context(scheduler.make_context()) {
	m_ecs = std::make_shared<rynx::ecs>();
	m_vfs = rynx::make_opaque_unique_ptr<rynx::filesystem::vfs>();
	m_context->set_resource(*m_vfs);
	m_context->set_resource(m_ecs);
	m_context->set_resource(m_scenes);
}

void rynx::application::simulation::generate_tasks(float dt) {
	m_logic.generate_tasks(*m_context, dt);
}

void rynx::application::simulation::clear() {
	m_ecs->clear();
	m_logic.clear(*m_context);
}