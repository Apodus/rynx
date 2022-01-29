
#include <rynx/application/application.hpp>
#include <rynx/menu/Component.hpp>
#include <rynx/graphics/camera/camera.hpp>

class SampleApplication : public ::rynx::application::Application {
private:
	rynx::menu::System m_menuSystem;
	rynx::reflection::reflections m_reflections;

public:

	SampleApplication();
	virtual void startup_load() override;
	virtual void set_resources() override;
	virtual void set_simulation_rules() override;
};