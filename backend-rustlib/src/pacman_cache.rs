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

use crate::ffi::PackageView;

use std::collections::HashSet;

use alpm::Alpm;

pub fn get_list_of_packages(handle: &Alpm) -> Vec<PackageView> {
    let mut resulting_list: Vec<PackageView> = vec![];
    let mut candidates: HashSet<String> = HashSet::new();

    let localdb = handle.localdb();
    let dbs = handle.syncdbs();

    for db in dbs {
        for pkg in db.pkgs() {
            let pkgname = pkg.name();
            let pkgver = pkg.version();
            let pkgdesc = pkg.desc().unwrap_or("");

            // we assume first occurrence is the most latest(same as pacman does), so skip all
            // others
            if candidates.contains(pkgname) {
                continue;
            }

            // check if local version is lower than syncdb
            let upgradable = if let Ok(local_pkg) = localdb.pkg(pkgname) {
                local_pkg.version() < pkgver
            } else {
                false
            };

            candidates.insert(pkgname.into());
            resulting_list.push(PackageView {
                name: pkgname.to_string(),
                version: pkgver.to_string(),
                desc: pkgdesc.to_string(),
                upgradable,
            });
        }
    }

    resulting_list
}
