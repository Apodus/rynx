
using Sharpmake;


public class RynxProject : Project
{
	public RynxProject()
	{
		AddTargets(
			new Target(
				Platform.win64,
				DevEnv.vs2022,
				Optimization.Debug | Optimization.Release | Optimization.Retail
			)
		);

		SourceFilesIncludeRegex.Add(@".ixx");
	}

	[Configure()]
	public virtual void conf_rynx_project(Project.Configuration conf, Target target)
	{
		// compiler settings
		{
			conf.Defines.Add("_ENABLE_EXTENDED_ALIGNED_STORAGE");
			conf.Options.Add(Options.Vc.General.WindowsTargetPlatformVersion.v10_0_22621_0);

			if (target.Optimization == Optimization.Retail)
			{
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.FloatingPointModel.Fast);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.FloatingPointExceptions.Disable);
			}
			else
			{
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.FloatingPointModel.Fast);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.FloatingPointExceptions.Disable);
			}

			conf.Options.Add(Sharpmake.Options.Vc.Compiler.RTTI.Enable);
			conf.Options.Add(Sharpmake.Options.Vc.General.CharacterSet.Unicode);

			conf.Options.Add(Sharpmake.Options.Vc.Compiler.Exceptions.Enable);
			conf.Options.Add(Sharpmake.Options.Makefile.Compiler.Exceptions.Enable);
			conf.Options.Add(Sharpmake.Options.Vc.Compiler.CppLanguageStandard.Latest);

			conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4324")); // 'struct_name' : structure was padded due to __declspec(align())
			conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4251")); // 'class member': class 'type' needs to have dll-interface to be used by clients of class

			conf.Options.Add(Sharpmake.Options.Vc.Compiler.MultiProcessorCompilation.Enable);
			conf.Options.Add(Sharpmake.Options.Vc.Compiler.FiberSafe.Enable);

			if (target.Optimization == Optimization.Release)
			{
				conf.Defines.Add("RYNX_PROFILING_ENABLED_");
			}

			if (target.Platform == Sharpmake.Platform.win64)
			{
				// conf.Options.Add(Sharpmake.Options.Vc.Compiler.EnhancedInstructionSet.SIMD2);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.EnhancedInstructionSet.AdvancedVectorExtensions2);
			}

			if (target.Optimization != Optimization.Debug)
			{
				conf.Defines.Add("NDEBUG");
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.Optimization.FullOptimization);
				conf.Options.Add(Sharpmake.Options.Vc.General.UseDebugLibraries.Disabled);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDLL);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.BufferSecurityCheck.Disable);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.FunctionLevelLinking.Enable);
			}
			else
			{
				conf.Defines.Add("_DEBUG");
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.MinimalRebuild.Enable);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.RuntimeChecks.Both);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.RuntimeLibrary.MultiThreadedDebugDLL);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.Optimization.Disable);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.BufferSecurityCheck.Enable);
			}

			if (target.Optimization == Optimization.Retail)
			{
				conf.Options.Add(Sharpmake.Options.Vc.Linker.EnableCOMDATFolding.RemoveRedundantCOMDATs);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.OmitFramePointers.Enable);
				conf.Options.Add(Sharpmake.Options.Vc.Linker.Reference.EliminateUnreferencedData);
				conf.Options.Add(Sharpmake.Options.Vc.General.WholeProgramOptimization.LinkTime);
				conf.Options.Add(Sharpmake.Options.Vc.Linker.LinkTimeCodeGeneration.UseLinkTimeCodeGeneration);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.Inline.AnySuitable);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.FavorSizeOrSpeed.FastCode);
			}
			else
			{
				conf.Options.Add(Sharpmake.Options.Vc.Linker.EnableCOMDATFolding.DoNotRemoveRedundantCOMDATs);
				conf.Options.Add(Sharpmake.Options.Vc.Linker.Reference.KeepUnreferencedData);
				conf.Options.Add(Sharpmake.Options.Vc.General.WholeProgramOptimization.Disable);
				conf.Options.Add(Sharpmake.Options.Vc.Linker.Incremental.Enable);
				conf.Options.Add(Sharpmake.Options.Vc.Compiler.Inline.OnlyInline);
				conf.Defines.Add("RYNX_ASSERTS_ENABLED_");
			}
		}

		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\src\");

		if(false) {
			conf.Output = Project.Configuration.OutputType.Dll;
			conf.ExportDefines.Add("[project.Name]DLL=__declspec(dllimport)");
			conf.Defines.Add("[project.Name]DLL=__declspec(dllexport)");
		}
		else {
			conf.Output = Project.Configuration.OutputType.Lib;
			conf.ExportDefines.Add("[project.Name]DLL=");
			conf.Defines.Add("[project.Name]DLL=");
		}
		
		conf.ProjectPath = @"[project.SharpmakeCsPath]/../generate/build/projects/";

		conf.IntermediatePath = @"[conf.ProjectPath]/intermediate/[project.Name]_[target.Name]";
		conf.TargetLibraryPath = @"[project.SharpmakeCsPath]\..\build_temp\lib_[target.Optimization]";
		conf.SolutionFolder = "Rynx";

		conf.TargetFileName = Name;
		conf.ProjectFileName = "[project.Name]_[target.DevEnv]";
		conf.TargetPath = @"[conf.ProjectPath]/output/[project.Name]_[target.Name]";
	}
}

public class TestProject : RynxProject
{
	public TestProject() { }

	[Configure()]
	public void ConfigureRynxProject(Configuration conf, Target target)
	{
		conf.SolutionFolder = "Rynx\\Tests";
		conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\external\catch2\");
		conf.Output = Configuration.OutputType.Exe;

		conf.TargetFileName = Name;
		conf.TargetPath = @"[project.SharpmakeCsPath]\..\build\tests\";
		conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings()
		{ LocalDebuggerWorkingDirectory = conf.TargetPath };
	}
}

class ExternalProject : RynxProject
{
	public ExternalProject() { }

	[Configure()]
	public void conf_external(Project.Configuration conf, Target target)
	{
		conf.Output = Project.Configuration.OutputType.Lib;
		if (target.Platform == Platform.win64)
			conf.Options.Add(Options.Vc.General.WarningLevel.Level0); // don't care about external warnings
		conf.SolutionFolder = "External";
	}
}