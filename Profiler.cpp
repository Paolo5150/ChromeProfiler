#include "Profiler.h"
#include <Windows.h>
#include <ctime>
#include <iostream>

std::unique_ptr<Profiler> Profiler::m_instance;

//ScopeEvent
ScopeEvent::ScopeEvent(const char* name)
{
	auto now = std::chrono::high_resolution_clock::now();

	m_info.Category = "Scope";
	m_info.EventName = name;
	m_info.EventType = 'B';
	m_info.TimeStart = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
	m_info.ProcessID = static_cast<uint32_t>(GetCurrentProcessId());
	m_info.ThreadID = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));

	Profiler::Instance().WriteInfo(m_info);
}

ScopeEvent::~ScopeEvent()
{
	auto now = std::chrono::high_resolution_clock::now();

	m_info.Category = "Scope";
	m_info.EventType = 'E';
	m_info.TimeStart = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
	m_info.ProcessID = static_cast<uint32_t>(GetCurrentProcessId());
	m_info.ThreadID = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
	Profiler::Instance().WriteInfo(m_info);
}

// Custom event
CustomEvent::CustomEvent(const char* name, bool async) : m_isAsync(async)
{
	auto now = std::chrono::high_resolution_clock::now();

	m_info.Category = "Custom";
	m_info.EventName = name;
	m_info.EventType = async? 'b' : 'B';
	m_info.TimeStart = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
	m_info.ProcessID = static_cast<uint32_t>(GetCurrentProcessId());
	m_info.ThreadID = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
	m_info.Id = reinterpret_cast<std::uintptr_t>(this);

	Profiler::Instance().WriteInfo(m_info);
}
CustomEvent::~CustomEvent()
{
	auto now = std::chrono::high_resolution_clock::now();

	m_info.Category = "Custom";
	m_info.EventType = m_isAsync ? 'e' : 'E';
	m_info.TimeStart = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
	m_info.ProcessID = static_cast<uint32_t>(GetCurrentProcessId());
	m_info.ThreadID = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
	Profiler::Instance().WriteInfo(m_info);
}

// Instant Event
InstantEvent::InstantEvent(const char* name)
{
	auto now = std::chrono::high_resolution_clock::now();

	m_info.EventName = name;
	m_info.Category = "Instant";
	m_info.EventType = 'i';

	m_info.ProcessID = static_cast<uint32_t>(GetCurrentProcessId());
	m_info.ThreadID = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
	m_info.TimeStart = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
	m_info.Scope = 't'; //Default is thread scope
}


InstantEvent::~InstantEvent()
{
	Profiler::Instance().WriteInfo(m_info);
}

Profiler& Profiler::Instance()
{
	if (m_instance == nullptr)
		m_instance = std::make_unique < Profiler>();

	return *m_instance;
}

Profiler::~Profiler()
{
	if (m_thread)
		m_thread->join();
}


//Profiler
void Profiler::StartSession(const std::string& sessionName)
{
	// If a previos session was sterted, make sure to join the thread
	if (m_thread)
	{
		m_thread->join();
		m_thread.reset();
	}

	m_threadRunning = true;
	m_writeComma = false;
	m_thread = std::make_unique<std::thread>(&Profiler::ThreadJob, this, sessionName);
}

void Profiler::EndSession()
{
	std::lock_guard l(m_outstreamMutex);
	m_threadRunning = false;
	m_waitCondition.notify_all();
}

CustomEvent* Profiler::StartCustomAsyncEvent(const std::string& eventName)
{
	std::lock_guard l(m_asyncEventMapMutex);

	auto ce = new CustomEvent(eventName.c_str(), true);
	m_customAsyncEvents[eventName] = ce;
	return ce;
}

CustomEvent* Profiler::GetCustomAsyncEvent(const std::string& eventName)
{
	std::lock_guard l(m_asyncEventMapMutex);

	if (m_customAsyncEvents.find(eventName) != m_customAsyncEvents.end())
		return m_customAsyncEvents[eventName];

	return nullptr;
}

void Profiler::EndCustomAsyncEvent(const std::string& eventName)
{
	std::lock_guard l(m_asyncEventMapMutex);

	if (m_customAsyncEvents.find(eventName) != m_customAsyncEvents.end())
	{
		delete m_customAsyncEvents[eventName];
		m_customAsyncEvents.erase(eventName);
	}
}

CustomEvent* Profiler::StartCustomEvent(const std::string& eventName)
{
	auto ce = new CustomEvent(eventName.c_str());
	m_customEvents[eventName] = ce;
	return ce;

}

CustomEvent* Profiler::GetCustomEvent(const std::string& eventName)
{
	if (m_customEvents.find(eventName) != m_customEvents.end())
		return m_customEvents[eventName];

	return nullptr;
}

void Profiler::EndCustomEvent(const std::string& eventName)
{
	if (m_customEvents.find(eventName) != m_customEvents.end())
	{
		delete m_customEvents[eventName];
		m_customEvents.erase(eventName);
	}
}

void Profiler::ThreadJob(std::string sessionName)
{
	std::ofstream m_outStream;

	std::time_t t = std::time(0);   // get time now
	std::tm* now = std::localtime(&t);

	std::stringstream ss;
	ss << sessionName << "_" << now->tm_mday << "-" << now->tm_mon << "-" << (now->tm_year + 1900) <<
		"_" << now->tm_hour << "-" << now->tm_min << "-" << now->tm_sec << ".json";

	m_outStream.open(ss.str());
	m_outStream << "[";
	m_outStream.flush();
	m_isSessionActive = true;

	while (true)
	{
		if (m_isSessionActive)
		{
			std::unique_lock<std::mutex> l(m_outstreamMutex);
			m_waitCondition.wait(l, [&] {return !m_eventQueue.empty()|| !m_threadRunning; });

			while (!m_eventQueue.empty())
			{
				auto info = m_eventQueue.front();
				m_eventQueue.pop();
				if (m_writeComma)
				{
					m_outStream << ",\n";
				}

				m_outStream << "{";
				m_outStream << "\"name\": \"" << info.EventName << "\",";
				m_outStream << "\"cat\": \"" << info.Category << "\",";
				m_outStream << "\"ph\": \"" << info.EventType << "\",";
				m_outStream << "\"pid\": " << info.ProcessID << ",";
				m_outStream << "\"tid\": " << info.ThreadID << ",";
				m_outStream << "\"ts\": " << info.TimeStart;

				if (info.Id.has_value())
					m_outStream << ", \"id\": " << info.Id.value();

				if (info.Scope.has_value())
					m_outStream << ", \"s\": \"" << info.Scope.value() << "\"";

				if (info.Args.size() > 0)
				{
					m_outStream << ",\"args\": {";

					bool first = true;
					for (auto it = info.Args.begin(); it != info.Args.end(); it++)
					{
						if (!first)
							m_outStream << ",";
						m_outStream << "\"" << it->first << "\": \"" << it->second << "\"";
						first = false;
					}
					m_outStream << "}";

				}

				m_outStream << "}";

				m_writeComma = true;

				m_outStream.flush();
			}
		
		}
		else
		{
			m_outStream << "Error: no session was started!\n";
		}			

		if (!m_threadRunning && m_eventQueue.empty()) break;
	}

	m_isSessionActive = false;
	m_outStream << "]";
	m_outStream.flush();

	m_outStream.close();	
}


void Profiler::WriteInfo(const ProfileEventInfo& info)
{
	std::unique_lock<std::mutex> l(m_outstreamMutex);
	m_eventQueue.push(info);	
	m_waitCondition.notify_all();
}