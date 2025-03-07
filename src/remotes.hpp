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

#ifndef REMOTES_HPP
#define REMOTES_HPP

#include "cmd.hpp"

#include <QComboBox>
#include <QDialog>
#include <QLineEdit>

class ManageRemotes : public QDialog {
    Q_OBJECT
 public:
    explicit ManageRemotes(QWidget* parent = nullptr);
    void listFlatpakRemotes() const noexcept;

    [[nodiscard]] bool is_changed() const noexcept { return m_changed; }
    [[nodiscard]] const auto& get_install_ref() const noexcept { return m_install_ref; }
    [[nodiscard]] const auto& get_user() const noexcept { return m_user; }

 signals:

 public slots:
    void removeItem() noexcept;
    void addItem() noexcept;
    void setInstall() noexcept;
    void userSelected(int index) noexcept;

 private:
    bool m_changed{};
    Cmd* m_cmd{};
    QComboBox* m_combo_remote;
    QComboBox* m_combo_user;
    QLineEdit* m_edit_add_remote;
    QLineEdit* m_edit_install_flatpakref;
    QString m_user{"--system "};
    QString m_install_ref{};
};

#endif  // REMOTES_HPP
