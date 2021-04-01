
using Sharpmake;

[Generate]
class stb : ExternalProject
{
	public stb()
	{
		SourceRootPath = @"[project.SharpmakeCsPath]\..\external\stb\";
	}

	[Configure]
	public void ConfigureAll(Project.Configuration conf, Target target)
	{
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\stb\");
	}
}

[Generate]
class GLEW : ExternalProject
{
	public GLEW()
	{
		SourceRootPath = @"[project.SharpmakeCsPath]\..\external\glew-2.1.0\src";
	}

	[Configure]
	public void ConfigureAll(Project.Configuration conf, Target target)
	{
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\glew-2.1.0\include");
		conf.Defines.Add("GLEW_STATIC");
		conf.ExportDefines.Add("GLEW_STATIC");
		conf.SourceFilesBuildExclude.Add("glewinfo.c");
		conf.SourceFilesBuildExclude.Add("visualinfo.c");
		conf.LibraryFiles.Add("opengl32.lib");
	}
}

[Generate]
class GLFW : ExternalProject
{
	public GLFW()
	{
		SourceRootPath = @"[project.SharpmakeCsPath]\..\external\glfw-3.3\src";
	}

	[Configure]
	public void ConfigureAll(Project.Configuration conf, Target target)
	{
		if (target.Platform == Platform.win64)
		{
			conf.SourceFilesFiltersRegex.Add("[/\\\\]win32.*c");
			conf.SourceFilesFiltersRegex.Add("[/\\\\]context.c");
			conf.SourceFilesFiltersRegex.Add("[/\\\\]egl_context.c");
			conf.SourceFilesFiltersRegex.Add("[/\\\\]init.c");
			conf.SourceFilesFiltersRegex.Add("[/\\\\]input.c");
			conf.SourceFilesFiltersRegex.Add("[/\\\\]monitor.c");
			conf.SourceFilesFiltersRegex.Add("[/\\\\]osmesa_context.c");
			conf.SourceFilesFiltersRegex.Add("[/\\\\]wgl_context.c");
			conf.SourceFilesFiltersRegex.Add("[/\\\\]window.c");
			conf.SourceFilesFiltersRegex.Add("[/\\\\]vulkan.c");

			conf.Defines.Add("WIN32");
			conf.Defines.Add("_WINDOWS");
			conf.Defines.Add("_CRT_SECURE_NO_WARNINGS");
		}

		conf.Defines.Add("_GLFW_USE_CONFIG_H");
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\glfw-3.3\include");
	}
}

[Generate]
class portaudio : ExternalProject
{
	public portaudio()
	{
		SourceRootPath = @"[project.SharpmakeCsPath]\..\external\portaudio\src";
	}

	[Configure]
	public void ConfigureAll(Project.Configuration conf, Target target)
	{
		if (target.Platform == Platform.win64)
		{
			// conf.SourceFilesFiltersRegex.Add(".*win.*");
			conf.SourceFilesBuildExcludeRegex.Add(".*mac_.*");
			conf.SourceFilesBuildExcludeRegex.Add(".*linux.*");
			conf.SourceFilesBuildExcludeRegex.Add(".*unix.*");
			conf.SourceFilesBuildExcludeRegex.Add(".*alsa.*");
			conf.SourceFilesBuildExcludeRegex.Add(".*pa_jack.*");
			conf.SourceFilesBuildExcludeRegex.Add(".*pa_asio.*");
			conf.SourceFilesBuildExcludeRegex.Add(".*recplay.*");
		}

		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\portaudio\src\common");
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\portaudio\src\hostapi");
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\portaudio\src\os");
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\portaudio\src\os\win");

		conf.Defines.Add("PA_USE_WASAPI=1");
		// conf.Defines.Add("PA_USE_ASIO=0");
		// conf.Defines.Add("PA_USE_DS=0");
		// conf.Defines.Add("PA_USE_WDMKS=0");
		// conf.Defines.Add("PA_USE_SKELETON=0");
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\portaudio\include");
	}
}

[Generate]
class libogg : ExternalProject
{
	public libogg()
	{
		SourceRootPath = @"[project.SharpmakeCsPath]\..\external\libogg-1.3.4\src";
	}

	[Configure]
	public void ConfigureAll(Project.Configuration conf, Target target)
	{
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\libogg-1.3.4\include");
	}
}

[Generate]
class libvorbis : ExternalProject
{
	public libvorbis()
	{
		SourceRootPath = @"[project.SharpmakeCsPath]\..\external\libvorbis-1.3.6\lib";
	}

	[Configure]
	public void ConfigureAll(Project.Configuration conf, Target target)
	{
		conf.Defines.Add("_USRDLL");
		conf.Defines.Add("LIBVORBIS_EXPORTS");
		conf.AddPublicDependency<libogg>(target);
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\libvorbis-1.3.6\include");

		conf.SourceFilesBuildExcludeRegex.Add(".*tone.c");
		conf.SourceFilesBuildExcludeRegex.Add(".*psytune.c");
	}
}