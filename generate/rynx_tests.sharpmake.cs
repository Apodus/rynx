
using Sharpmake;

[Generate]
public class TestScheduler : TestProject
{
    public TestScheduler()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\test\scheduler\";
    }
	
	[Configure]
    public void conf_test(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<Scheduler>(target);
    }
}

[Generate]
public class TestTech : TestProject
{
    public TestTech()
    {
        SourceRootPath = @"[project.SharpmakeCsPath]\..\src\test\tech\";
    }
	
	[Configure]
    public void conf_test(Project.Configuration conf, Target target)
    {
		conf.AddPublicDependency<Tech>(target);
	}
}