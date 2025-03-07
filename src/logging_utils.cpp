#include "logging_utils.hpp"

#include <spdlog/spdlog.h>

namespace utils {

void log_error_msg(const std::string& message) noexcept {
    spdlog::error("{}", message);
}

void log_warn_msg(const std::string& message) noexcept {
    spdlog::warn("{}", message);
}

void log_info_msg(const std::string& message) noexcept {
    spdlog::info("{}", message);
}

}  // namespace utils
