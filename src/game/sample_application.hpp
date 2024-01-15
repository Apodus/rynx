
#include <rynx/application/application.hpp>
#include <rynx/menu/Component.hpp>
#include <rynx/graphics/camera/camera.hpp>

class SampleApplication : public ::rynx::application::Application {
private:
	rynx::menu::System m_menuSystem;
	rynx::reflection::reflections m_reflections;

	rynx::binary_config::id m_editorRunning;
	rynx::binary_config::id m_gameRunning;

public:
	rynx::binary_config::id get_binary_config_is_editor_running() { return m_editorRunning; }
	rynx::binary_config::id get_binary_config_is_game_running() { return m_gameRunning; }

	SampleApplication();
	virtual void startup_load() override;
	virtual void set_resources() override;
	virtual void set_simulation_rules() override;

	rynx::reflection::reflections& reflection() { return m_reflections; }
};