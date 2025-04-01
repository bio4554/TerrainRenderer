#include "pch.h"
#include <format>
#include "DMLogger.h"

#include <chrono>
#include <iostream>

namespace dm
{
	LogCallback Logger::_logCallback = nullptr;

	void Logger::information(std::string message, std::string _context)
	{
		print_header(_context);

		if (_logCallback != nullptr)
		{
			_logCallback(std::format("{}\n", message).c_str());
			return;
		}

		std::cout << std::format("{}\n", message);
	}

	void Logger::warning(std::string message, std::string _context)
	{
		print_header(_context);

		if (_logCallback != nullptr)
		{
			_logCallback(std::format("{}\n", message).c_str());
			return;
		}

		std::cout << std::format("{}\n", message);
	}

	void Logger::set_log_callback(LogCallback callback)
	{
		_logCallback = callback;
	}

	std::string Logger::vec3_print(const glm::vec3& vec)
	{
		return std::format("x: {} y: {} z: {}", vec.x, vec.y, vec.z);
	}

	void Logger::error(std::string message, std::string _context)
	{
		print_header(_context);

		if (_logCallback != nullptr)
		{
			_logCallback(std::format("{}\n", message).c_str());
			return;
		}

		std::cout << std::format("{}\n", message);
	}

	void Logger::print_header(std::string _context)
	{
		auto now = std::chrono::system_clock::now();

		if (_logCallback != nullptr)
		{
			auto message = std::format("[{}] ", now) + std::format("{}: ", _context);
			_logCallback(message.c_str());
			return;
		}

		std::cout << std::format("[{}] ", now);
		std::cout << std::format("{}: ", _context);
	}

	void Logger::_print(std::string message)
	{
		if (_logCallback != nullptr)
		{
			_logCallback(message.c_str());
		}
		else
		{

		}
	}

	LoggerContext::LoggerContext(std::string context)
	{
		_context = context;
	}

	void LoggerContext::information(std::string message)
	{
		Logger::information(message, _context);
	}

	void LoggerContext::warning(std::string message)
	{
		Logger::warning(message, _context);
	}

	void LoggerContext::error(std::string message)
	{
		Logger::error(message, _context);
	}


}
