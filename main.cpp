#include <chrono>
#include "Profiler.h"
#include <iostream>

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
		auto tid = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

		PROFILE_CUSTOM_ASYNC_END("Multithread event", "ThreadStopID", tid);
		Sleep(125);

		int totalTicks = 12;
		int ticks = totalTicks;
		int tickId = 0;
		auto prev = std::chrono::high_resolution_clock::now();

		while (ticks > 0)
		{
			PROFILE_SCOPE("Async frame");
			PROFILE_INSTANT("Tick Start", "TickID", tickId, "TotalTicks", totalTicks);

			auto now = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(now - prev).count() / 1000;
			Update(deltaTime);
			Render();
			ticks--;
			tickId++;
		}

		});

	int totalTicks = 18;
	int ticks = totalTicks;
	int tickId = 0;

	auto prev = std::chrono::high_resolution_clock::now();
	auto tid = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
	PROFILE_CUSTOM_ASYNC_START("Multithread event", "ThreadStartID", tid); //Event closes in other thread

	while (ticks > 0)
	{
		PROFILE_SCOPE("Frame", "TickID", tickId, "TotalTicks", totalTicks);

		auto now = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(now - prev).count() / 1000;
		Update(deltaTime);
		Render();
		ticks--;
		tickId++;

		if (tickId == 2)
		{
			// If nested with a scope/func event, a custom event must start and end within it
			// Ensure to use the same string for starting and ending
			PROFILE_CUSTOM_START("Custom event", "Some random key", 1224);
			Sleep(26);
			PROFILE_CUSTOM_END("Custom event", "Another random key", "A random value");
		}
		

		// Custom async event doens't need to be properly nested with other events (like scope)
		// Ensure to use the same string for starting and ending
		if(tickId == 1)
			PROFILE_CUSTOM_ASYNC_START("Async Custom event");
		if (tickId == 5)
			PROFILE_CUSTOM_ASYNC_END("Async Custom event");

	}
	t.join();
	PROFILE_END();

	PROFILE_BEGIN("Other");
	InitFunction();

	{
		PROFILE_SCOPE("Empty");
		Sleep(93);
	}
	PROFILE_END();
	return 0;
}