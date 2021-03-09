using System.IO;
using Sharpmake;

[module: Sharpmake.Include("types.sharpmake.cs")]

[Generate]
class Codegen : RynxProject
{
    public Codegen()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\tools\rynx-codegen\";
    }
    
    [Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
	conf.IncludePaths.Add(@"[project.SharpmakeCsPath]\..\tools\rynx-codegen\external\llvm-11.0.0\include\");
	conf.LibraryFiles.Add(@"[project.SharpmakeCsPath]\..\tools\rynx-codegen\external\llvm-11.0.0\lib\libclang.lib");

	conf.TargetFileName = "rynx-codegen";
	conf.SolutionFolder = "";
	conf.TargetPath = @"[project.SharpmakeCsPath]\..\tools\rynx-codegen\";
	conf.Output = Project.Configuration.OutputType.Exe;
	
	conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings()
		{ LocalDebuggerWorkingDirectory = conf.TargetPath };
    }
}

[Generate]
class RynxCodegen : Solution
{
    public RynxCodegen()
    {
        AddTargets(new Target(
            Platform.win64,
            DevEnv.vs2019,
            Optimization.Debug | Optimization.Release));
    }

    [Configure]
    public void ConfigureAll(Solution.Configuration conf, Target target)
    {
        conf.SolutionPath = @"[solution.SharpmakeCsPath]\..";
        conf.AddProject<Codegen>(target);
    }
}

public class EntryPoint
{
    [Sharpmake.Main]
    public static void SharpmakeMain(Sharpmake.Arguments sharpmakeArgs)
    {
	sharpmakeArgs.Generate<RynxCodegen>();
    }
}