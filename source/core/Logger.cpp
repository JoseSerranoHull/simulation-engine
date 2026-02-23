#include "core/Logger.h"

/* parasoft-begin-suppress ALL */
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <vector>
#include <iostream>
#include <ctime>
#include <cstring>
/* parasoft-end-suppress ALL */

namespace GE::Utilities {
	namespace fs = std::filesystem;

	LogLevel Logger::s_logLevel = LogLevel::Info | LogLevel::Debug | LogLevel::Warn | LogLevel::Error;
	std::ofstream Logger::s_file;
	std::mutex Logger::s_mutex;
	bool Logger::s_initialized = false;
	std::string Logger::s_logFilePath;
	size_t Logger::s_maxFiles = 1;
	uint64_t Logger::s_maxFileSizeBytes = 5ull * 1024 * 1024;

	ERROR_CODE Logger::Initialize(const std::string &logFilePath, size_t maxFiles, uint64_t maxFileSizeBytes)
	{
		std::lock_guard<std::mutex> lk(s_mutex);
		if (s_initialized)
			return ERROR_CODE::ALREADY_INITIALIZED;

		s_logFilePath = logFilePath;
		s_maxFiles = (maxFiles == 0) ? 1 : maxFiles;
		s_maxFileSizeBytes = (maxFileSizeBytes == 0) ? (5ull * 1024 * 1024) : maxFileSizeBytes;


		RotateAtInitialize();


		s_file.open(s_logFilePath, std::ios::out | std::ios::trunc);
		s_initialized = s_file.is_open();

		if (!s_initialized)
		{
			std::cerr << "Logger::Initialize failed to open log file: " << s_logFilePath << std::endl;
		}
		else
		{
			s_file << "=== Log started: " << CurrentTimestampString() << " ===\n";
			s_file.flush();
		}
		return ERROR_CODE::OK;
	}

	ERROR_CODE Logger::Shutdown()
	{
		std::lock_guard<std::mutex> lk(s_mutex);
		if (s_file.is_open())
		{
			s_file.flush();
			s_file.close();
		}
		s_initialized = false;

		return ERROR_CODE::OK;
	}

	std::string Logger::FormatMessage(LogLevel level, const std::string &msg, const char *file, int line)
	{
		using namespace std::chrono;
		auto now = system_clock::now();
		auto t = system_clock::to_time_t(now);


		std::tm tm;
		if (localtime_s(&tm, &t) != 0) {
			std::memset(&tm, 0, sizeof(std::tm));
		}

		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		oss << " ["
			<< (level == LogLevel::Debug  ? "DBG"
				: level == LogLevel::Info ? "INF"
				: level == LogLevel::Warn ? "WRN"
										  : "ERR")
			<< "] ";

		if (file)
		{
			const char *p = file;
			const char *slash = strrchr(p, '\\');
			if (!slash)
				slash = strrchr(p, '/');
			oss << (slash != nullptr ? (slash + 1) : p) << ":" << line << " - ";
		}

		oss << msg;
		return oss.str();
	}

	std::string Logger::CurrentTimestampString()
	{
		using namespace std::chrono;
		auto now = system_clock::now();
		auto t = system_clock::to_time_t(now);

		std::tm tm;
		if (localtime_s(&tm, &t) != 0) {
			std::memset(&tm, 0, sizeof(std::tm));
		}

		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
		return oss.str();
	}

	void Logger::Log(LogLevel level, const std::string &message, const char *file, int line)
	{

		if (!(s_logLevel & level))
			return;

		std::string formatted = FormatMessage(level, message, file, line);

		std::lock_guard<std::mutex> lk(s_mutex);

		if (!s_initialized)
		{


			if (!s_initialized)
			{

				s_file.open(s_logFilePath.empty() ? "engine.log" : s_logFilePath, std::ios::out | std::ios::app);
				s_initialized = s_file.is_open();
			}
		}

		if (s_initialized && s_file.is_open())
		{

			try
			{
				RotateIfNeededLocked();
			}
			catch (...)
			{

			}

			s_file << formatted << "\n";
			s_file.flush();
		}



		std::cerr << formatted << std::endl;
	}

	bool Logger::RotateAtInitialize()
	{
		if (s_maxFiles <= 1)
		{

			return true;
		}

		try
		{
			fs::path base = s_logFilePath;
			if (!fs::exists(base))
				return true;


			std::string backup = BackupFileNameWithTimestampLocked();
			fs::path backupPath = fs::path(backup);
			std::error_code ec;
			fs::rename(base, backupPath, ec);



			CleanupOldBackupsLocked();
			return true;
		}
		catch (const std::exception &e)
		{
			std::string msg = std::string("Logger::RotateAtInitialize exception: ") + e.what();
			std::cerr << msg << std::endl;
			return false;
		}
	}

	void Logger::RotateIfNeededLocked()
	{

		if (s_maxFileSizeBytes == 0 || s_maxFiles <= 1)
			return;

		try
		{
			fs::path base = s_logFilePath;
			if (!fs::exists(base))
				return;

			uint64_t sz = (uint64_t)fs::file_size(base);
			if (sz >= s_maxFileSizeBytes)
			{

				RotateNowLocked();

				CleanupOldBackupsLocked();
			}
		}
		catch (...)
		{

		}
	}

	bool Logger::RotateNowLocked()
	{

		try
		{

			if (s_file.is_open())
			{
				s_file.flush();
				s_file.close();
			}

			fs::path base = s_logFilePath;
			std::string backup = BackupFileNameWithTimestampLocked();
			fs::path backupPath = fs::path(backup);
			std::error_code ec;
			fs::rename(base, backupPath, ec);
			(void)ec;


			s_file.open(s_logFilePath, std::ios::out | std::ios::trunc);
			s_initialized = s_file.is_open();

			if (s_initialized)
			{
				s_file << "=== Log rotated: " << CurrentTimestampString() << " ===\n";
				s_file.flush();
			}
			else
			{
				std::cerr << "Logger::RotateNowLocked failed to open new log file" << std::endl;
			}
			return true;
		}
		catch (const std::exception &e)
		{
			std::string msg = std::string("Logger::RotateNowLocked exception: ") + e.what();
			std::cerr << msg << std::endl;
			return false;
		}
	}

	void Logger::CleanupOldBackupsLocked()
	{

		if (s_maxFiles <= 1)
			return;

		try
		{
			fs::path base = s_logFilePath;
			fs::path dir = base.parent_path();
			if (dir.empty())
				dir = ".";

			std::string baseName = base.filename().string();


			std::vector<fs::directory_entry> backups;
			for (auto &entry : fs::directory_iterator(dir))
			{
				if (!entry.is_regular_file())
					continue;
				std::string name = entry.path().filename().string();

				if (name.size() > baseName.size() + 1 && name.compare(0, baseName.size(), baseName) == 0 &&
					name[baseName.size()] == '.')
				{
					backups.push_back(entry);
				}
			}


			std::sort(backups.begin(), backups.end(),
					  [](const fs::directory_entry &a, const fs::directory_entry &b)
					  {
						  std::error_code ea, eb;
						  auto ta = fs::last_write_time(a.path(), ea);
						  auto tb = fs::last_write_time(b.path(), eb);
						  if (ea || eb)
					  		return a.path().string() < b.path().string();
						  return ta < tb;
					  });


			size_t allowed = (s_maxFiles > 0) ? (s_maxFiles - 1) : 0;
			while (backups.size() > allowed)
			{
				auto toDelete = backups.front();
				std::error_code ec;
				fs::remove(toDelete.path(), ec);
				backups.erase(backups.begin());
			}
		}
		catch (...)
		{

		}
	}

	std::string Logger::BackupFileNameWithTimestampLocked()
	{

		fs::path base = s_logFilePath;
		std::string ts = CurrentTimestampString();
		std::string backup = base.string() + "." + ts;
		return backup;
	}
}