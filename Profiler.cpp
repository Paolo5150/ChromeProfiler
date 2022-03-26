#include "Profiler.h"
#include <Windows.h>
#include <ctime>
#include <iostream>

std::unique_ptr<Profiler> Profiler::m_instance;

//ScopeEvent
ScopeEvent::ScopeEvent(const char* name)
{
	m_start = std::chrono::high_resolution_clock::now();

	m_info.Category = "Scope";
	m_info.EventName = name;
	m_info.EventType = 'X';
	m_info.ProcessID = static_cast<uint32_t>(GetCurrentProcessId());
	m_info.ThreadID = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

ScopeEvent::~ScopeEvent()
{
	auto now = std::chrono::high_resolution_clock::now();

	long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_start).time_since_epoch().count();

	auto ms_int = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count();
	m_info.TimeStart = start;
	m_info.TimeDuration = ms_int;

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
	if(m_instance == nullptr)
		m_instance = std::make_unique < Profiler>();

	return *m_instance;
}

//Profiler
void Profiler::StartSession(const std::string& sessionName)
{
	std::time_t t = std::time(0);   // get time now
	std::tm* now = std::localtime(&t);

	std::stringstream ss;
	ss << sessionName << "_" << now->tm_mday << "-" << now->tm_mon << "-" << (now->tm_year + 1900) << 
		"_" << now->tm_hour << "-" <<now->tm_min << "-" << now->tm_sec <<".json";

	m_outStream.open(ss.str());
	m_outStream << "[";
	m_outStream.flush();
	m_isSessionActive = true;
}

void Profiler::EndSession()
{
	m_isSessionActive = false;
	m_outStream << "]";
	m_outStream.flush();
}

void Profiler::WriteInfo(const ProfileEventInfo& info)
{
	if (m_isSessionActive)
	{
		if (m_writeComma)
		{
			m_outStream << ",\n";
		}
		m_outStream << "{";
		m_outStream << "\"name\": \"" << info.EventName << "\",";
		m_outStream << "\"cat\": \"" << info.Category << "\",";
		m_outStream << "\"ph\": \"" << info.EventType <<"\",";
		m_outStream << "\"pid\": " << info.ProcessID << ",";
		m_outStream << "\"tid\": " << info.ThreadID << ",";
		m_outStream << "\"ts\": " << info.TimeStart << ",";

		if(info.TimeDuration.has_value())
			m_outStream << "\"dur\": " << info.TimeDuration.value();
		if (info.Scope.has_value())
			m_outStream << "\"s\": \"" << info.Scope.value() << "\"";

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
	}
	else
	{
		m_outStream << "Error: no session was started!\n";
	}
}
