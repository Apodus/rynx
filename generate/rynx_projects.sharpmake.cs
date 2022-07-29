
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
class Math : RynxProject
{
    public Math()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\math\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
        conf.AddPublicDependency<RynxSystem>(target);
		conf.AddPublicDependency<RynxStd>(target);
    }
}

[Generate]
class Editor : RynxProject
{
    public Editor()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\editor\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
        conf.AddPublicDependency<Menu>(target);
        conf.AddPublicDependency<Tech>(target);
        conf.AddPublicDependency<Scheduler>(target);
		conf.AddPublicDependency<RynxStd>(target);
		conf.AddPublicDependency<Application>(target);
		conf.AddPublicDependency<Graphics>(target);
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
		conf.AddPublicDependency<Graphics>(target); // because graphics is responsible for opening window, and input comes from the native window.
		// TODO: Create a new "NativeWindow" project, so input doesnt depend on graphics?
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
		conf.AddPublicDependency<Graphics>(target);
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
		conf.AddPublicDependency<Graphics>(target);
		conf.AddPublicDependency<Input>(target);
		conf.AddPublicDependency<Menu>(target);
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
        conf.AddPublicDependency<RynxStd>(target);
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
        conf.AddPublicDependency<RynxStd>(target);
		conf.AddPublicDependency<Profiling>(target);
        conf.AddPublicDependency<Math>(target);
		conf.AddPublicDependency<Reflection>(target);
		conf.AddPublicDependency<Ecs>(target);
    }
}

[Generate]
class Ecs : RynxProject
{
    public Ecs()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\ecs\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
        conf.AddPublicDependency<RynxStd>(target);
		conf.AddPublicDependency<Profiling>(target);
        conf.AddPublicDependency<Math>(target);
		conf.AddPublicDependency<Reflection>(target);
    }
}

[Generate]
class Reflection : RynxProject
{
    public Reflection()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\reflection\";
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
        conf.AddPublicDependency<RynxStd>(target);
		conf.AddPublicDependency<Profiling>(target);
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
        conf.AddPublicDependency<Math>(target);
		conf.AddPublicDependency<FileSystem>(target);
		conf.AddPublicDependency<RynxStd>(target);
		conf.AddPublicDependency<Profiling>(target);
		conf.AddPublicDependency<Scheduler>(target);
		conf.AddPublicDependency<Ecs>(target);
    }
}

[Generate]
class RynxStd : RynxProject
{
    public RynxStd()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\std\";
        AdditionalSourceRootPaths.Add(@"[project.SharpmakeCsPath]/../tools/natvis/");
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
        conf.AddPublicDependency<RynxSystem>(target);
    }
}

[Generate]
class Profiling : RynxProject
{
    public Profiling()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\profiling\";
        AdditionalSourceRootPaths.Add(@"[project.SharpmakeCsPath]/../tools/natvis/");
    }

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
        conf.AddPublicDependency<RynxStd>(target);
		conf.AddPublicDependency<FileSystem>(target);
		conf.AddPublicDependency<Thread>(target);
    }
}

[Generate]
class FileSystem : RynxProject
{
    public FileSystem()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\rynx\filesystem\";
        AdditionalSourceRootPaths.Add(@"[project.SharpmakeCsPath]/../tools/natvis/");
	}

    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<RynxStd>(target);
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