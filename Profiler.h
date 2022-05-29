#pragma once
#include <string>
#include <fstream>
#include <thread>
#include <map>
#include <optional>
#include <sstream>
#include <queue>
#include <condition_variable>
#include <mutex>

#define PROFILE_ON //Comment this out to disable all profiling

struct ProfileEventInfo
{
	std::string EventName;
	std::string Category;
	uint32_t ProcessID;
	uint32_t ThreadID;
	char EventType;
	long long TimeStart;
	long long TimeEnd;
	std::map<std::string, std::string> Args;

	std::optional<long long> TimeDuration;
	std::optional<char> Scope; // Used for instant event
	std::optional<std::uintptr_t> Id; //Used for custom event
};

class ProfileEvent
{
public:
	template<typename... Arguments>
	void AddArgs(Arguments &&...args)
	{
		int v[] = { 0, ((void)ForEach(std::forward<Arguments>(args)), 0) ... };
		(void)v;
	}

protected:
	ProfileEventInfo m_info;

private:

	template <typename T>
	void ForEach(T t) {
		if (m_counter % 2 == 0) //it's a key, cache
		{
			if constexpr (std::is_same_v<std::decay<T>::type, std::string>)
			{
				m_currentKey = t;
			}
		}
		else //it's a value, push
		{
			std::stringstream ss;
			ss << t;
			m_info.Args[m_currentKey] = ss.str();
		}
		m_counter++;
	}
	int m_counter = 0;
	std::string m_currentKey;
};

class ScopeEvent : public ProfileEvent
{
public:
	ScopeEvent(const char* name);
	~ScopeEvent();
};

class CustomEvent : public ProfileEvent
{
public:
	CustomEvent(const char* name, bool async = false);
	~CustomEvent();
private:
	bool m_isAsync;
};

class InstantEvent : public ProfileEvent
{
public:
	friend class Profiler;

	InstantEvent(const char* name);
	~InstantEvent();
};

class Profiler
{
public:
	static Profiler& Instance();
	~Profiler();
	void StartSession(const std::string& sessionName = "Profile", bool useInfoConsoleLogs = false);
	void EndSession();
	void WriteInfo(const ProfileEventInfo& info);

	CustomEvent* StartCustomAsyncEvent(const std::string& eventName);
	void EndCustomAsyncEvent(const std::string& eventName);
	CustomEvent* GetCustomAsyncEvent(const std::string& eventName);

	CustomEvent* StartCustomEvent(const std::string& eventName);
	CustomEvent* GetCustomEvent(const std::string& eventName);
	void EndCustomEvent(const std::string& eventName);

private:
	static std::unique_ptr<Profiler> m_instance;

	std::map<std::string, CustomEvent*> m_customAsyncEvents; //In a separate map, as we need to lock if events are started/ended in different threads
	std::map<std::string, CustomEvent*> m_customEvents;
	void ThreadJob(std::string sessionName);

	bool m_isSessionActive;
	bool m_writeComma = false;
	bool m_threadRunning = false;
	bool m_useInternalCommandLogs = false;
	std::unique_ptr<std::thread> m_thread; // Writing thread
	std::queue< ProfileEventInfo> m_eventQueue; // List of events to write
	std::mutex m_outstreamMutex; // Used to lock list of events to write
	std::mutex m_asyncEventMapMutex; // Used to lock the map of async events
	std::condition_variable m_waitCondition; // Synchronize the list of events};
};

#ifdef PROFILE_ON
#define PROFILE_BEGIN(fileName) Profiler::Instance().StartSession(fileName)
#define PROFILE_BEGIN_WLOGS(fileName) Profiler::Instance().StartSession(fileName,true)
#define PROFILE_END() Profiler::Instance().EndSession()
#define PROFILE_FUNC(...) ScopeEvent __event__(__FUNCSIG__); __event__.AddArgs(__VA_ARGS__)
#define PROFILE_SCOPE(eventName,...) ScopeEvent __event__(eventName); __event__.AddArgs(__VA_ARGS__)
#define PROFILE_CUSTOM_ASYNC_START(eventName,...) Profiler::Instance().StartCustomAsyncEvent(eventName)->AddArgs(__VA_ARGS__)
#define PROFILE_CUSTOM_ASYNC_END(eventName,...){ Profiler::Instance().GetCustomAsyncEvent(eventName)->AddArgs(__VA_ARGS__); Profiler::Instance().EndCustomAsyncEvent(eventName);}
#define PROFILE_CUSTOM_START(eventName,...) Profiler::Instance().StartCustomEvent(eventName)->AddArgs(__VA_ARGS__)
#define PROFILE_CUSTOM_END(eventName,...) {Profiler::Instance().GetCustomEvent(eventName)->AddArgs(__VA_ARGS__); Profiler::Instance().EndCustomEvent(eventName); }
#define PROFILE_INSTANT(eventName,...) {InstantEvent __event__(eventName); __event__.AddArgs(__VA_ARGS__);}
#else
#define PROFILE_BEGIN(fileName)
#define PROFILE_END()
#define PROFILE_FUNC(...)
#define PROFILE_SCOPE(eventName,...)
#define PROFILE_CUSTOM_ASYNC_START(eventName,...)
#define PROFILE_CUSTOM_ASYNC_END(eventName,...)
#define PROFILE_CUSTOM_START(eventName,...)
#define PROFILE_CUSTOM_END(eventName,...)
#define PROFILE_INSTANT(eventName,...)
#endif