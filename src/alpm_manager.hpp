// Copyright (C) 2025 Vladislav Nepogodin
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#ifndef ALPM_MANAGER_HPP
#define ALPM_MANAGER_HPP

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wdeprecated-this-capture"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsuggest-final-types"
#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include "rust/cxx.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace cachyos_pi::rustlib {
struct AlpmManager;
}

namespace alpm {

struct PackageView {
    std::string name;
    std::string pkgver;
    std::string desc;
    bool upgradable;
};

class AlpmManager {
 public:
    /// @brief Initializes and configures ALPM library.
    static auto init_alpm_manager() noexcept -> std::optional<AlpmManager>;

    /// @brief Reinitializes ALPM library.
    void refresh_alpm() noexcept;

    /// @brief Runs preparation for transactions.
    /// @param targets The vector of targets to be added to transaction.
    /// @param conflict_msg Reference to a string to store the message on conflict.
    /// @return Status code 0 on success, 1 overwise.
    auto sync_trans(const std::vector<std::string>& targets, std::string& conflict_msg) noexcept -> std::int32_t;

    /// @brief Adds targets to install transaction and prepare.
    /// @param targets The vector of targets to be added to install transaction.
    /// @param conflict_msg Reference to a string to store the message on conflict.
    /// @return Status code 0 on success, 1 overwise.
    auto prepare_add_trans(const std::vector<std::string>& targets, std::string& conflict_msg) noexcept -> std::int32_t;

    /// @brief Adds targets to remove transaction and prepare.
    /// @param targets The vector of targets to be added to remove transaction.
    /// @param conflict_msg Reference to a string to store the message on conflict.
    /// @return Status code 0 on success, 1 overwise.
    auto prepare_remove_trans(const std::vector<std::string>& targets, std::string& conflict_msg) noexcept -> std::int32_t;

    /// @brief Returns string containing targets which will be affected.
    /// @param targets The vector of targets to be added to install transaction.
    /// @param verbosepkglists Flag indicating whether the output should be verbose.
    /// @param status_text Reference to a string to store the status message.
    /// @return The affected targets.
    auto display_install_targets(const std::vector<std::string>& targets, bool verbosepkglists, std::string& status_text) noexcept -> std::string;

    /// @brief Returns string containing targets which will be affected.
    /// @param targets The vector of targets to be added to remove transaction.
    /// @param verbosepkglists Flag indicating whether the output should be verbose.
    /// @param status_text Reference to a string to store the status message.
    /// @return The affected targets.
    auto display_remove_targets(const std::vector<std::string>& targets, bool verbosepkglists, std::string& status_text) noexcept -> std::string;

    /// @brief Retrieves the view of package from syncdbs.
    /// @param pkgname The pkgname of the package to retrieve.
    /// @return An optional PackageView object if the package is available, std::nullopt otherwise.
    auto get_package_view(std::string_view pkgname) noexcept -> std::optional<PackageView>;

    /// @brief Retrieves the list of packages where present only packages from upper repository.
    /// @return An vector of PackageView objects if the packages available, empty vector otherwise.
    auto get_list_of_packages() noexcept -> std::vector<PackageView>;

    // explicitly deleted
    AlpmManager() = delete;

 private:
    explicit AlpmManager(::rust::Box<::cachyos_pi::rustlib::AlpmManager>&& manager) : m_manager(std::move(manager)) { }

    /// @brief A pointer to the AlpmManager structure from Rust code.
    ::rust::Box<::cachyos_pi::rustlib::AlpmManager> m_manager;
};

/// @brief A shared pointer to a AlpmManager object.
using AlpmManagerPtr = std::shared_ptr<AlpmManager>;

/// @brief Checks if ALPM dbs have at least single package.
auto is_valid_alpm_dbs() noexcept -> bool;

}  // namespace alpm

#endif  // ALPM_MANAGER_HPP
