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

#ifndef PACMANCACHE_HPP
#define PACMANCACHE_HPP

#include "alpm_manager.hpp"

#include <map>
#include <memory>
#include <utility>

#include <QStringList>

class PacmanCache {
 public:
    explicit PacmanCache(alpm::AlpmManagerPtr manager) : m_alpm_manager(std::move(manager)) { refresh_list(); }

    void refresh_list() noexcept;

    [[nodiscard]] const auto& get_candidates() const noexcept { return m_candidates; }
    [[nodiscard]] const auto& get_upgrade_candidates() const noexcept { return m_upd_candidates; }

 private:
    QStringList m_upd_candidates;
    std::map<QString, QStringList> m_candidates;
    alpm::AlpmManagerPtr m_alpm_manager{};
};

#endif  // PACMANCACHE_HPP
