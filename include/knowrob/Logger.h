/*
 * Copyright (c) 2022, Daniel Beßler
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_LOGGING_H_
#define KNOWROB_LOGGING_H_

// note: must be defined before including spdlog.h
//#ifdef _DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
//#else
//#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
//#endif

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
// BOOST
#include <boost/property_tree/ptree.hpp>

#define KB_TRACE    SPDLOG_TRACE
#define KB_DEBUG    SPDLOG_DEBUG
#define KB_INFO     SPDLOG_INFO
#define KB_WARN     SPDLOG_WARN
#define KB_ERROR    SPDLOG_ERROR
#define KB_CRITICAL SPDLOG_CRITICAL

#define KB_LOGGER_CALL(file, line, level, ...) \
	spdlog::default_logger_raw()->log(spdlog::source_loc{file, line, SPDLOG_FUNCTION}, level, __VA_ARGS__)

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#    define KB_TRACE1(file, line, ...) KB_LOGGER_CALL(file, line, spdlog::level::trace, __VA_ARGS__)
#else
#    define KB_TRACE(file, line, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#    define KB_DEBUG1(file, line, ...) KB_LOGGER_CALL(file, line, spdlog::level::debug, __VA_ARGS__)
#else
#    define KB_DEBUG(file, line, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#    define KB_INFO1(file, line, ...) KB_LOGGER_CALL(file, line, spdlog::level::info, __VA_ARGS__)
#else
#    define KB_INFO(file, line, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#    define KB_WARN1(file, line, ...) KB_LOGGER_CALL(file, line, spdlog::level::warn, __VA_ARGS__)
#else
#    define KB_WARN(file, line, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#    define KB_ERROR1(file, line, ...) KB_LOGGER_CALL(file, line, spdlog::level::err, __VA_ARGS__)
#else
#    define KB_ERROR(file, line, ...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#    define KB_CRITICAL1(file, line, ...) KB_LOGGER_CALL(file, line, spdlog::level::critical, __VA_ARGS__)
#else
#    define KB_CRITICAL(file, line, ...) (void)0
#endif

namespace knowrob {
	/**
	 * Interface to initialize and configure the default logger of the system.
	 * The default logger is used within the logging macros KB_INFO, KB_ERROR, ...
	 */
	class Logger {
	public:
		/**
		 * The type of a logging sink.
		 */
		enum SinkType { File, Console };

		/**
		 * Initialize the logging subsystem, i.e., configure it with default parameters.
		 * The configuration may be changed at a later point, e.g. when reading a settings file.
		 */
		static void initialize();

		/**
		 * Configure logging using a property tree.
		 * @param config a property tree.
		 */
		static void loadConfiguration(boost::property_tree::ptree &config);

		/**
		 * Create a new fie sink. Any existing file sink will be replaced
		 * by the new one.
		 * @param basename the base name of log files
		 * @param rotate true if files should be rotated on opening
		 * @param max_size max size of log files before rotation
		 * @param max_files max number of stored files
		 */
		static void setupFileSink(const std::string &basename="knowrob.log",
								  bool rotate=true,
								  uint32_t max_size=1048576,
								  uint32_t max_files=4);

		/**
		 * @param sinkType the type of the sink to configure.
		 * @param log_level the logging level for the console sink.
		 */
		static void setSinkLevel(SinkType sinkType, spdlog::level::level_enum log_level);

		/**
		 * @param sinkType the type of the sink to configure.
		 * @param pattern the logging pattern for the console sink.
		 */
		static void setSinkPattern(SinkType sinkType, const std::string &pattern);

		/**
		 * @param name the name of a component.
		 * @param type the type of action that failed.
		 * @return a formatted error message.
		 */
		static std::string formatGenericFailure(const std::string &name, const std::string &type);

	protected:
		// hide implementation details
		struct impl;
		std::unique_ptr<impl> pimpl_;

		Logger();

		void updateLogger();

		static Logger& get();
	};
}

/**
 * Catch any exception that `goal` may throw and log it as an error.
 */
#define KB_LOGGED_TRY_CATCH(name, type, goal) do { \
	try { goal } \
	catch (const boost::python::error_already_set&) \
	{ LOG_KNOWROB_ERROR(PythonError(), Logger::formatGenericFailure(name,type)); } \
	catch (KnowRobError &e) \
	{ LOG_KNOWROB_ERROR(e, Logger::formatGenericFailure(name,type)); } \
	catch (std::exception &e) \
	{ KB_ERROR("{}: {}", Logger::formatGenericFailure(name,type), e.what()); } \
	catch (...) \
	{ KB_ERROR("{}: unknown failure.", Logger::formatGenericFailure(name,type)); }  \
} while(0)

/**
 * Catch any exception that `goal` may throw and log it as an error.
 */
#define KB_LOGGED_TRY_EXCEPT(name, type, goal, except) do { \
	try { goal } \
	catch (const boost::python::error_already_set&) \
	{ LOG_KNOWROB_ERROR(PythonError(), Logger::formatGenericFailure(name,type)); except } \
	catch (KnowRobError &e) \
	{ LOG_KNOWROB_ERROR(e, Logger::formatGenericFailure(name,type)); except } \
	catch (std::exception &e) \
	{ KB_ERROR("{}: {}", Logger::formatGenericFailure(name,type), e.what()); except } \
	catch (...) \
	{ KB_ERROR("{}: unknown failure.", Logger::formatGenericFailure(name,type)); except }  \
} while(0)

#endif //KNOWROB_LOGGING_H_
