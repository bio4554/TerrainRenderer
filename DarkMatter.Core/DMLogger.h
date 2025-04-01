#pragma once

#include <string>
#include <glm/vec3.hpp>

namespace dm
{
	typedef void (*LogCallback)(const char*);

	class Logger
	{
	public:
		static void information(std::string message, std::string _context);
		static void error(std::string message, std::string _context);
		static void warning(std::string message, std::string _context);
		static void set_log_callback(LogCallback callback);
		static std::string vec3_print(const glm::vec3& vec);
	private:
		static LogCallback _logCallback;
		static void print_header(std::string _context);
		static void _print(std::string message);
	};

	class LoggerContext
	{
	public:
		LoggerContext(std::string context);

		void information(std::string message);
		void error(std::string message);
		void warning(std::string message);
	private:
		std::string _context;
	};
}
