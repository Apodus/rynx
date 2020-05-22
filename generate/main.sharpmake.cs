using System.IO;
using Sharpmake;

[module: Sharpmake.Include("types.sharpmake.cs")]
[module: Sharpmake.Include("external_projects.sharpmake.cs")]
[module: Sharpmake.Include("rynx_projects.sharpmake.cs")]
[module: Sharpmake.Include("rynx_tests.sharpmake.cs")]

[Generate]
class Game : RynxProject
{
    public Game()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\game\";
    }
	
	[Configure]
    public void ConfigureAll(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<RuleSets>(target);
		conf.AddPublicDependency<Input>(target);
		conf.AddPublicDependency<Menu>(target);
        conf.AddPublicDependency<Graphics>(target);
		conf.AddPublicDependency<Tech>(target);
		conf.AddPublicDependency<Scheduler>(target);
		
		conf.TargetFileName = Name;
		conf.SolutionFolder = ""; // game is not part of rynx. should not be rynxproject.
		conf.TargetPath = @"[project.SharpmakeCsPath]\..\build\bin\";
		conf.Output = Project.Configuration.OutputType.Exe;
		
		conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings()
            { LocalDebuggerWorkingDirectory = conf.TargetPath };
    }
}

[Generate]
class Rynx : Solution
{
    public Rynx()
    {
        AddTargets(new Target(
            Platform.win64,
            DevEnv.vs2019,
            Optimization.Debug | Optimization.Release | Optimization.Retail));
    }

    [Configure]
    public void ConfigureAll(Solution.Configuration conf, Target target)
    {
        conf.SolutionPath = @"[solution.SharpmakeCsPath]\..";
        conf.AddProject<Game>(target);
		
		conf.AddProject<TestTech>(target);
		conf.AddProject<TestScheduler>(target);
	}
}

public class EntryPoint
{
    [Sharpmake.Main]
    public static void SharpmakeMain(Sharpmake.Arguments sharpmakeArgs)
    {
        sharpmakeArgs.Generate<Rynx>();
    }
}