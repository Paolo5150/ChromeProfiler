# ChromeProfiler
Simple (and extendable) C++ profiler for chrome::tracing

# What is this?

Did you know that Google chrome has a tool that allows you to visualize profiling data?

Check: chrome://tracing/

# So?

This simple profiler for C++ allows you to quickly have some basic profiling data. 
You'll be able to see your function calls in a timeline, their execution time, process and thread IDs and you can add any argument you mey like to profiling calls.
It outputs a json and all you have to do is drop it in the tracing tool, using the link above.

It's very basic, but can be extended.

Documentation on the json format is here: https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/edit

# Example

All you have to use are on liners macro calls.

```
// Custom sleep
void Sleep(int milliseconds)
{
	auto now = std::chrono::high_resolution_clock::now();
	while (true)
	{
		auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now).count();
		if (delta >= milliseconds)
			break;

	}
}

void InitFunction()
{
	PROFILE_FUNC(); //Macro for quick scope profile of a function
	Sleep(52);
}

void Update(float deltaTime)
{
	// Profile funciton can take args, format is key:string, value: any primitive
	int r = rand() % 10 + 5;
	PROFILE_FUNC("Random", r, "DeltaTime", deltaTime);
	Sleep(r);
}

void Render()
{
	int r = rand() % 10 + 5;
	PROFILE_FUNC("Random", r);
	Sleep(r);
}

int main()
{
	srand((unsigned)time(nullptr));
	PROFILE_BEGIN("Profile");
	InitFunction();

	std::thread t([]() {
		
		PROFILE_SCOPE("MyThread"); //Name + any needed arg (key,value)
		Sleep(125);

		//Create instant event passing name, and then a bunch of optional key, value args
		PROFILE_INSTANT("Some event happened", "someKey", 12, "otherKey", "somevalue");
		Sleep(125);
		PROFILE_INSTANT("Some other thing happened, no args");

		Sleep(125);

		});

	int ticks = 16;
	auto prev = std::chrono::high_resolution_clock::now();

	while (ticks > 0)
	{
		auto now = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(now - prev).count() / 1000;
		Update(deltaTime);
		Render();
		ticks--;
	}
	t.join();
	PROFILE_END();
	return 0;
}
```
Dropping the generate json into chrome tracing, you should see something like:
![image](https://user-images.githubusercontent.com/8026573/160246473-284989cd-e7dd-4f50-ae9b-41e76b8c1a68.png)

