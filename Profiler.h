#pragma once
#include <string>
#include <fstream>
#include <thread>
#include <map>
#include <optional>
#include <sstream>

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
	std::optional<char> Scope;

};

class ProfileEvent
{
public:
	template<typename... Arguments>
	void AddArgs(Arguments &&...args)
	{
		int v[] = { 0, ((void)ForEach(std::forward<Arguments>(args)), 0) ... };
	}

protected:
	ProfileEventInfo m_info;

private:

	template <typename T>
	void ForEach(T t) {
		if (m_counter % 2 == 0) //it's a key, cache
		{
			m_currentKey = t;
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

private:
	std::chrono::high_resolution_clock::time_point m_start;
};


class InstantEvent : public ProfileEvent
{
public:
	InstantEvent(const char* name);
	~InstantEvent();
};

class Profiler
{
public:
	static Profiler& Instance();

	void StartSession(const std::string& sessionName);
	void EndSession();
	void WriteInfo(const ProfileEventInfo& info);

private:
	static std::unique_ptr<Profiler> m_instance;
	std::ofstream m_outStream;
	bool m_isSessionActive;
	bool m_writeComma = false;
	
};

#define PROFILE_ON

#ifdef PROFILE_ON
#define PROFILE_BEGIN(x) Profiler::Instance().StartSession(x)
#define PROFILE_END() Profiler::Instance().EndSession()
#define PROFILE_FUNC(...) ScopeEvent __event__(__func__); __event__.AddArgs(__VA_ARGS__)
#define PROFILE_SCOPE(x,...) ScopeEvent __event__(x); __event__.AddArgs(__VA_ARGS__)
#define PROFILE_INSTANT(x,...) {InstantEvent __event__(x); __event__.AddArgs(__VA_ARGS__);}

#else
#define PROFILE_BEGIN(x)
#define PROFILE_END()
#define PROFILE_FUNC()
#define PROFILE_SCOPE(x) 
#define PROFILE_INSTANT(x)
#endif




