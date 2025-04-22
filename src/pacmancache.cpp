// Copyright (C) 2022-2025 Vladislav Nepogodin
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

#include "pacmancache.hpp"

#include <algorithm>  // for filter
#include <ranges>     // for ranges::*

void PacmanCache::refresh_list() noexcept {
    auto list_of_packages = m_alpm_manager->get_list_of_packages();

    // installed packages which have lower version than in syncdbs
    QStringList upd_candidates;
    for (auto package : list_of_packages | std::ranges::views::filter(&alpm::PackageView::upgradable)) {
        auto&& package_name = QString::fromStdString(package.name);
        upd_candidates << package_name;
    }
    m_upd_candidates = std::move(upd_candidates);

    // all syncdb packages
    for (auto&& package : list_of_packages) {
        auto&& package_name = QString::fromStdString(std::move(package.name));
        auto&& package_ver  = QString::fromStdString(std::move(package.pkgver));
        auto&& package_desc = QString::fromStdString(std::move(package.desc));

        m_candidates[package_name] = (QStringList() << package_ver << package_desc);
    }
}
