#include "logger.h"
#include <iostream>
#pragma warning(disable : 4996)

std::string logfile;
std::ofstream outFile;


std::string getCurrentTimestamp() {
	std::ostringstream ss;
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);
	ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
	return ss.str();
}

void logMessage(LogLevel_ level, const std::string& func, const std::string& message, const std::string& file, int line) {
	if (!outFile.is_open()) return;

	std::string levelStr;
	switch (level) {
	case LogLevel_::INFO_: levelStr = "INF"; break;
	case LogLevel_::WARNING_: levelStr = "WAR"; break;
	case LogLevel_::ERROR_: levelStr = "ERR"; break;
	case LogLevel_::DEBUG_: levelStr = "DEB"; break;
	case LogLevel_::TRACE_: levelStr = "TRA"; break;
	}

	outFile << "[" << getCurrentTimestamp() << "]\t[" << levelStr << "]\t["
		<< func << "]\t(" << file << ":" << line << ")\t" << message << std::endl;

}

void setLogFile(const char* filename){
	logfile = filename;
	outFile.open(logfile, std::ios::out | std::ios::app);
	if (!outFile) {
		std::cerr << "Failed to open log file: " << logfile << std::endl;
	}
}

void closeLogFile() {
	if (outFile.is_open()) {
		outFile.close();
	}
}