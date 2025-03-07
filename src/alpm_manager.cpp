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

#include "alpm_manager.hpp"

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

#include "backend-rustlib-cxxbridge/lib.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <spdlog/spdlog.h>

namespace {

auto convert_std_vec_into_rustvec(const std::vector<std::string>& std_vec) -> ::rust::Vec<::rust::String> {
    ::rust::Vec<::rust::String> vec;
    vec.reserve(std_vec.size());
    for (auto&& vec_el : std_vec) {
        vec.push_back(::rust::String(vec_el));
    }
    return vec;
}

auto convert_rust_vec_packageview(auto&& rust_vec) -> std::vector<alpm::PackageView> {
    std::vector<alpm::PackageView> res_vec{};
    res_vec.reserve(rust_vec.size());
    for (auto&& pkgview_rust : rust_vec) {
        auto package_view = alpm::PackageView{.name = std::string(pkgview_rust.name), .pkgver = std::string(pkgview_rust.version), .desc = std::string(pkgview_rust.desc), .upgradable = pkgview_rust.upgradable};
        res_vec.emplace_back(std::move(package_view));
    }
    return res_vec;
}

}  // namespace

namespace alpm {

auto AlpmManager::init_alpm_manager() noexcept -> std::optional<AlpmManager> {
    try {
        auto&& manager = AlpmManager(cachyos_pi::rustlib::init_alpm_manager());
        return std::make_optional<AlpmManager>(std::move(manager));
    } catch (const std::exception& e) {
        spdlog::error("failed to init ALPM handle: {}", e.what());
    }
    return std::nullopt;
}

void AlpmManager::refresh_alpm() noexcept {
    try {
        m_manager->refresh();
    } catch (const std::exception& e) {
        spdlog::error("failed to refresh ALPM handle: {}", e.what());
    }
}

auto AlpmManager::sync_trans(const std::vector<std::string>& targets, std::string& conflict_msg) noexcept -> std::int32_t {
    try {
        const auto targets_rust     = convert_std_vec_into_rustvec(targets);
        const auto conflictmsg_rust = m_manager->sync_trans_pub(targets_rust);
        conflict_msg                = std::string(conflictmsg_rust);
    } catch (const std::exception& e) {
        spdlog::error("failed to sync trans: {}", e.what());
        return 1;
    }
    return 0;
}

auto AlpmManager::prepare_add_trans(const std::vector<std::string>& targets, std::string& conflict_msg) noexcept -> std::int32_t {
    try {
        const auto targets_rust     = convert_std_vec_into_rustvec(targets);
        const auto conflictmsg_rust = m_manager->prepare_add_trans_pub(targets_rust);
        conflict_msg                = std::string(conflictmsg_rust);
    } catch (const std::exception& e) {
        spdlog::error("failed to prepare install trans: {}", e.what());
        return 1;
    }
    return 0;
}

auto AlpmManager::prepare_remove_trans(const std::vector<std::string>& targets, std::string& conflict_msg) noexcept -> std::int32_t {
    try {
        const auto targets_rust     = convert_std_vec_into_rustvec(targets);
        const auto conflictmsg_rust = m_manager->prepare_remove_trans_pub(targets_rust);
        conflict_msg                = std::string(conflictmsg_rust);
    } catch (const std::exception& e) {
        spdlog::error("failed to prepare remove trans: {}", e.what());
        return 1;
    }
    return 0;
}

auto AlpmManager::display_install_targets(const std::vector<std::string>& targets, bool verbosepkglists, std::string& status_text) noexcept -> std::string {
    try {
        const auto targets_rust   = convert_std_vec_into_rustvec(targets);
        const auto strstruct_rust = m_manager->display_install_targets_pub(targets_rust, verbosepkglists);
        status_text               = std::string(strstruct_rust.first);
        return std::string(strstruct_rust.second);
    } catch (const std::exception& e) {
        spdlog::error("failed to display install targets: {}", e.what());
        return {};
    }
    return {};
}

auto AlpmManager::display_remove_targets(const std::vector<std::string>& targets, bool verbosepkglists, std::string& status_text) noexcept -> std::string {
    try {
        const auto targets_rust   = convert_std_vec_into_rustvec(targets);
        const auto strstruct_rust = m_manager->display_remove_targets_pub(targets_rust, verbosepkglists);
        status_text               = std::string(strstruct_rust.first);
        return std::string(strstruct_rust.second);
    } catch (const std::exception& e) {
        spdlog::error("failed to display remove targets: {}", e.what());
        return {};
    }
    return {};
}

auto AlpmManager::get_package_view(std::string_view pkgname) noexcept -> std::optional<PackageView> {
    try {
        const ::rust::Str pkgname_rust(pkgname.data(), pkgname.size());
        const auto pkgview_rust = m_manager->get_package_view(pkgname_rust);
        // for package view func upgradable is always false
        auto package_view = PackageView{.name = std::string(pkgview_rust.name), .pkgver = std::string(pkgview_rust.version), .desc = std::string(pkgview_rust.desc), .upgradable = pkgview_rust.upgradable};
        return std::make_optional<PackageView>(std::move(package_view));
    } catch (const std::exception& e) {
        spdlog::error("failed to get package view({}): {}", pkgname, e.what());
    }
    return std::nullopt;
}

auto AlpmManager::get_list_of_packages() noexcept -> std::vector<PackageView> {
    try {
        auto packages_rust = m_manager->get_list_of_packages();
        return convert_rust_vec_packageview(std::move(packages_rust));
    } catch (const std::exception& e) {
        spdlog::error("failed to get list of packages: {}", e.what());
    }
    return {};
}

auto is_valid_alpm_dbs() noexcept -> bool {
    try {
        return cachyos_pi::rustlib::is_valid_alpm_dbs();
    } catch (const std::exception& e) {
        spdlog::error("failed to check if ALPM is valid: {}", e.what());
    }
    return false;
}

}  // namespace alpm
