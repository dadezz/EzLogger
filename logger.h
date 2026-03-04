/**
 * @file    logger.h
 * @brief   Simple header-only, thread-safe logging system (C++20).
 * @author  Davide Zambon
 * 
 * Provides a macro-style interface (LOG_INFO, LOG_DEBUG, ...) implemented as
 * plain inline functions using std::source_location. no real preprocessor macros.
 *
 * Usage:
 *   if (auto err = setLogFile("app.log")) { return *err; }
 *   setDebugLogLevel();                    // optional, default is INFO_
 *   LOG_INFO("application started");
 *   LOG_DEBUG("verbose info");
 *   closeLogFile();                        // explicit close, or let RAII handle it
 *
 * Log levels (inclusive, setting a level enables that level and everything more severe):
 *   TRACE_ < DEBUG_ < INFO_ < WARNING_ < ERROR_
 *
 * Thread safety: all public methods are protected by a mutex.
 */

#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <optional>
#include <source_location>

enum class LogLevel_;
class Logger;

 // ── Macro-styled interface ───────────────────────────────────────────────────────────
 // Capture call-site info and forward to the Logger singleton. These are the main functions to use in client code.

inline std::optional<std::string> setLogFile(std::string_view filename) { return Logger::instance().setFile(filename); }
inline void                       closeLogFile() { Logger::instance().close(); }
inline void                       setInfoLogLevel() { Logger::instance().setLevel(LogLevel_::INFO_); }
inline void                       setWarningLogLevel() { Logger::instance().setLevel(LogLevel_::WARNING_); }
inline void                       setErrorLogLevel() { Logger::instance().setLevel(LogLevel_::ERROR_); }
inline void                       setDebugLogLevel() { Logger::instance().setLevel(LogLevel_::DEBUG_); }
inline void                       setTraceLogLevel() { Logger::instance().setLevel(LogLevel_::TRACE_); }

inline void LOG_ERROR(std::string_view msg, const std::source_location& loc = std::source_location::current()) { Logger::instance().log(LogLevel_::ERROR_, msg, loc); }
inline void LOG_WARN(std::string_view msg, const std::source_location& loc = std::source_location::current()) { Logger::instance().log(LogLevel_::WARNING_, msg, loc); }
inline void LOG_INFO(std::string_view msg, const std::source_location& loc = std::source_location::current()) { Logger::instance().log(LogLevel_::INFO_, msg, loc); }
inline void LOG_DEBUG(std::string_view msg, const std::source_location& loc = std::source_location::current()) { Logger::instance().log(LogLevel_::DEBUG_, msg, loc); }
inline void LOG_TRACE(std::string_view msg, const std::source_location& loc = std::source_location::current()) { Logger::instance().log(LogLevel_::TRACE_, msg, loc); }

// ── Log levels ────────────────────────────────────────────────────────────────
// Numeric values are meaningful: a message is printed only if its level value
// is >= the active level value (see Logger::log).
enum class LogLevel_ {
    TRACE_ = 0,  // very verbose, e.g., function entry/exit, variable values
    DEBUG_ = 1,  // debug info, verbose
    INFO_ = 2,  // general info (default active level)
    WARNING_ = 3,  // recoverable anomalies
    ERROR_ = 4,  // non-recoverable errors
};

// ── Logger class definition ─────────────────────────────────────────────────
/// Thread-safe logging singleton with a C-style macro interface.
/// Constructed on first use (Meyer's singleton); destroyed at program exit (RAII).
class Logger {
public:
    /// Returns the single global instance
    static Logger& instance() {
        static Logger inst;
        return inst;
    }

    // Non-copyable, non-movable.
    Logger(const Logger&)               = delete;
    Logger& operator=(const Logger&)    = delete;
    Logger(Logger&&)                    = delete;
    Logger& operator=(Logger&&)         = delete;

    /// Opens @p filename for appending. Creates the file if it does not exist.
    /// @return std::nullopt on success, or an error message string on failure.
    std::optional<std::string> setFile(std::string_view filename) {
        std::scoped_lock lock(mutex_);
        if (outFile_.is_open()) {
            outFile_.close();
		}
        logfile_ = filename;
        outFile_.open(logfile_, std::ios::out | std::ios::app);
        if (!outFile_) {
            std::string err = "[Logger] Failed to open log file: " + logfile_;
            return err;
        }
        return std::nullopt;
    }

    /// Flushes and closes the log file. Called automatically by the destructor.
    void close() {
        std::scoped_lock lock(mutex_);
        if (outFile_.is_open())
            outFile_.close();
    }

    /// Sets the minimum verbosity level. Messages below this level are discarded.
    /// Safe to call at any time, including from a different thread.
    void setLevel(LogLevel_ level) {
        std::scoped_lock lock(mutex_);
        activeLevel_ = level;
    }
    

    /// Writes a log entry.
    /// source_location default argument is evaluated at the call site, so
    /// function name, file, and line always refer to the caller — not this method.
    void log(LogLevel_ level,
        std::string_view message,
        const std::source_location& loc = std::source_location::current()) {

        if (level < activeLevel_) return;

        std::scoped_lock lock(mutex_);
        if (!outFile_.is_open()) return;

        const char* levelStr = "???";
        switch (level) {
        case LogLevel_::INFO_:    levelStr = "INF"; break;
        case LogLevel_::WARNING_: levelStr = "WAR"; break;
        case LogLevel_::ERROR_:   levelStr = "ERR"; break;
        case LogLevel_::DEBUG_:   levelStr = "DEB"; break;
        case LogLevel_::TRACE_:   levelStr = "TRA"; break;
        }

        outFile_ << '[' << getCurrentTimestamp() << "]\t["
            << levelStr << "]\t["
            << loc.function_name() << "]\t("
            << loc.file_name() << ':' << loc.line() << ")\t"
            << message << '\n';
        if (level == LogLevel_::ERROR_) {
            outFile_ << std::flush;
		}
    }

private:
    Logger() = default;
    ~Logger() { close(); }  // RAII: file is closed even if closeLogFile() is never called.

    /// Returns the current local time as "YYYY-MM-DD HH:MM:SS".
    /// thread-safe.
    static std::string getCurrentTimestamp() {
        std::ostringstream ss;
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        struct tm buf {};
#ifdef _WIN32
        localtime_s(&buf, &in_time_t);  // MSVC thread-safe variant
#else
        localtime_r(&in_time_t, &buf);  // POSIX thread-safe variant
#endif
        ss << std::put_time(&buf, "%Y-%m-%d %X");
        return ss.str();
    }

    std::string   logfile_;
    std::ofstream outFile_;
    std::atomic<LogLevel_> activeLevel_{LogLevel_::INFO_};  // default: DEBUG_ and TRACE_ are silent
    std::mutex    mutex_;
};
