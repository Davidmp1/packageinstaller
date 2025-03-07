#ifndef LOGGING_UTILS_HPP
#define LOGGING_UTILS_HPP

#include <string>

namespace utils {

/// @brief Logs message if error level is set.
void log_error_msg(const std::string& message) noexcept;

/// @brief Logs message if warn level is set.
void log_warn_msg(const std::string& message) noexcept;

/// @brief Logs message if info level is set.
void log_info_msg(const std::string& message) noexcept;

}  // namespace utils

#endif  // LOGGING_UTILS_HPP
