
using Sharpmake;

[Generate]
class Graphics : RynxProject
{
    public Graphics()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\graphics\";
    }
	
	[Configure]
	public void ConfigureAll(Project.Configuration conf, Target target)
	{
		conf.AddPublicDependency<GLEW>(target);
		conf.AddPublicDependency<GLFW>(target);
		conf.AddPublicDependency<stb>(target);
		conf.AddPublicDependency<Tech>(target);
	}
}

[Generate]
class Input : RynxProject
{
    public Input()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\input\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<Tech>(target);
		conf.AddPublicDependency<GLFW>(target);
    }
}

[Generate]
class Audio : RynxProject
{
	public Audio()
	{
		SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\audio";
	}
	
	[Configure]
	public void ConfigureAll(Project.Configuration conf, Target target)
	{
		conf.AddPrivateDependency<libvorbis>(target);
		conf.AddPrivateDependency<portaudio>(target);
		conf.AddPublicDependency<Tech>(target);
		conf.AddPublicDependency<RynxSystem>(target);
		conf.AddPublicDependency<Thread>(target);
	}
}

[Generate]
class Menu : RynxProject
{
    public Menu()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\menu\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<Tech>(target);
		conf.AddPublicDependency<Input>(target);
    }
}

[Generate]
class Application : RynxProject
{
    public Application()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\application\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<Audio>(target);
		conf.AddPublicDependency<Tech>(target);
    }
}

[Generate]
class RuleSets : RynxProject
{
    public RuleSets()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\rulesets\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<Tech>(target);
		conf.AddPublicDependency<Application>(target);
    }
}

[Generate]
class Thread : RynxProject
{
    public Thread()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\thread\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<RynxSystem>(target);
		conf.AddPublicDependency<Tech>(target);
	}
}

[Generate]
class Scheduler : RynxProject
{
    public Scheduler()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\scheduler\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<Thread>(target);
		conf.AddPublicDependency<Tech>(target);
	}
}

[Generate]
class Tech : RynxProject
{
    public Tech()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\tech\";
		AdditionalSourceRootPaths.Add(@"[project.SharpmakeCsPath]/../tools/natvis/");
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<RynxSystem>(target);
    }
}

[Generate]
class RynxSystem : RynxProject
{
    public RynxSystem()
    {
		Name = "System";
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\system\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
    }
}