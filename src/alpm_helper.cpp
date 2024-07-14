// Copyright (C) 2022-2024 Vladislav Nepogodin
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

#include "alpm_helper.hpp"
#include "ini.hpp"
#include "utils.hpp"

#include <sys/utsname.h>

#include <memory>

#include <fmt/core.h>
#include <spdlog/spdlog.h>

namespace {

struct pm_target_t {
    alpm_pkg_t* remove;
    alpm_pkg_t* install;
    std::int32_t is_explicit;
};

/* callback to handle messages/notifications from libalpm */
void cb_event(void* ctx, alpm_event_t* event);

/* callback to handle messages/notifications from pacman library */
__attribute__((format(printf, 3, 0))) void cb_log(void* ctx, alpm_loglevel_t level, const char* fmt, va_list args);

void cb_event(void* ctx, alpm_event_t* event) {
    (void)ctx;

    switch (event->type) {
    case ALPM_EVENT_CHECKDEPS_START:
        spdlog::info("ALPM: checking dependencies...");
        break;
    case ALPM_EVENT_RESOLVEDEPS_START:
        spdlog::info("ALPM: resolving dependencies...");
        break;
    case ALPM_EVENT_INTERCONFLICTS_START:
        spdlog::info("ALPM: looking for conflicting packages...");
        break;
    case ALPM_EVENT_TRANSACTION_START:
        spdlog::info("ALPM: :: Processing package changes...");
        break;

    case ALPM_EVENT_FILECONFLICTS_START:
        spdlog::info("ALPM: ALPM_EVENT_FILECONFLICTS_START");
        break;
    case ALPM_EVENT_FILECONFLICTS_DONE:
        spdlog::info("ALPM: ALPM_EVENT_FILECONFLICTS_DONE");
        break;
    case ALPM_EVENT_INTERCONFLICTS_DONE:
        spdlog::info("ALPM: ALPM_EVENT_INTERCONFLICTS_DONE");
        break;

    // all the simple done events, with fallthrough for each
    case ALPM_EVENT_KEY_DOWNLOAD_START:
    case ALPM_EVENT_SCRIPTLET_INFO:
    case ALPM_EVENT_PKG_RETRIEVE_START:
    case ALPM_EVENT_OPTDEP_REMOVAL:
    case ALPM_EVENT_DB_RETRIEVE_START:
    case ALPM_EVENT_DATABASE_MISSING:
    case ALPM_EVENT_PACKAGE_OPERATION_DONE:
    case ALPM_EVENT_DB_RETRIEVE_DONE:
    case ALPM_EVENT_DB_RETRIEVE_FAILED:
    case ALPM_EVENT_PKG_RETRIEVE_DONE:
    case ALPM_EVENT_PKG_RETRIEVE_FAILED:
    case ALPM_EVENT_HOOK_START:
    case ALPM_EVENT_HOOK_RUN_START:
    case ALPM_EVENT_PACKAGE_OPERATION_START:
    case ALPM_EVENT_DISKSPACE_START:
    case ALPM_EVENT_LOAD_START:
    case ALPM_EVENT_KEYRING_START:
    case ALPM_EVENT_INTEGRITY_START:
    case ALPM_EVENT_PACNEW_CREATED:
    case ALPM_EVENT_PACSAVE_CREATED:
    case ALPM_EVENT_CHECKDEPS_DONE:
    case ALPM_EVENT_RESOLVEDEPS_DONE:
    case ALPM_EVENT_TRANSACTION_DONE:
    case ALPM_EVENT_INTEGRITY_DONE:
    case ALPM_EVENT_KEYRING_DONE:
    case ALPM_EVENT_KEY_DOWNLOAD_DONE:
    case ALPM_EVENT_LOAD_DONE:
    case ALPM_EVENT_DISKSPACE_DONE:
    case ALPM_EVENT_HOOK_RUN_DONE:
    case ALPM_EVENT_HOOK_DONE:
        /* nothing */
        return;
    }
}

void cb_log(void* ctx, alpm_loglevel_t level, const char* fmt, va_list args) {
    (void)ctx;
    if (!fmt || strlen(fmt) == 0) {
        return;
    }

    if (level == ALPM_LOG_DEBUG || level == ALPM_LOG_FUNCTION) {
        return;
    }

    std::vector<char> temp{};
    std::size_t length{63};
    while (temp.size() <= length) {
        temp.resize(length + 1);
        const auto status = std::vsnprintf(temp.data(), temp.size(), fmt, args);
        if (status < 0) {
            return;
        }
        length = static_cast<std::size_t>(status);
    }
    const auto converted_str = std::string{temp.data(), length};
    const auto out_str       = fmt::format("ALPM: {}", converted_str);
    switch (level) {
    case ALPM_LOG_ERROR:
        spdlog::error("{}", out_str);
        break;
    case ALPM_LOG_WARNING:
        spdlog::warn("{}", out_str);
        break;
    default:
        break;
    }
}

void parse_repos(alpm_handle_t* handle) noexcept {
    static constexpr auto pacman_conf_path = "/etc/pacman.conf";

    const mINI::INIFile file(pacman_conf_path);
    // next, create a structure that will hold data
    mINI::INIStructure ini;

    // now we can read the file
    file.read(ini);
    for (const auto& it : ini) {
        const auto& section = it.first;
        const auto& nested  = it.second;
        if (section == "options") {
            for (const auto& it_nested : nested) {
                if (it_nested.first != "architecture") {
                    continue;
                }
                // add CacheDir
                const auto& archs = ::utils::make_multiline(it_nested.second, ' ');
                for (const auto& arch : archs) {
                    if (arch == "auto") {
                        alpm_list_t* physical_arches;
                        physical_arches = alpm_option_get_physical_architectures(handle);

                        for (alpm_list_t* i = physical_arches; i; i = alpm_list_next(i)) {
                            auto* physical_arch = static_cast<char*>(i->data);
                            if (physical_arch == nullptr) {
                                continue;
                            }
                            alpm_option_add_architecture(handle, physical_arch);
                        }
                        alpm_list_free(physical_arches);
                        break;
                    }

                    alpm_option_add_architecture(handle, arch.c_str());
                }
            }
            continue;
        }
        alpm_register_syncdb(handle, section.c_str(), ALPM_SIG_USE_DEFAULT);
    }
}

/* prepare a list of pkgs to display */
auto _display_targets(const std::vector<pm_target_t>& targets, bool verbose, std::string& status_text) noexcept -> std::string {
    if (targets.empty()) {
        return {};
    }

    std::string res{};
    off_t isize{};
    off_t rsize{};
    off_t dlsize{};

    /* gather package info */
    for (const auto& target : targets) {
        if (target.install != nullptr) {
            dlsize += alpm_pkg_download_size(target.install);
            isize += alpm_pkg_get_isize(target.install);
        }

        if (target.remove != nullptr) {
            /* add up size of all removed packages */
            rsize += alpm_pkg_get_isize(target.remove);
        }
    }

    /* form data for both verbose and non-verbose display */
    for (const auto& target : targets) {
        if (target.install != nullptr) {
            res += fmt::format("{}-{}", alpm_pkg_get_name(target.install), alpm_pkg_get_version(target.install));
        }
        if (target.remove != nullptr) {
            res += fmt::format("{}-{} [removal]", alpm_pkg_get_name(target.remove), alpm_pkg_get_version(target.remove));
        }

        res += (verbose) ? '\n' : ' ';
    }

    // presentation
    const auto& proper_present = [](auto&& number) {
        if (number < 1024) {
            return fmt::format("{}", number);
        } else if (number < 1024 * 1024) {
            return fmt::format("{} KB", number / 1024);
        } else if (number < 1024 * 1024 * 1024) {
            return fmt::format("{} MB", number / 1024 / 1024);
        }

        return fmt::format("{} GB", number / 1024 / 1024 / 1024);
    };

    if (dlsize > 0) {
        status_text += fmt::format("Total Download Size: {}\n", proper_present(dlsize));
    }

    if (isize > 0) {
        status_text += fmt::format("Total Installed Size: {}\n", proper_present(isize));
    }
    if (rsize > 0 && isize == 0) {
        status_text += fmt::format("Total Removed Size: {}\n", proper_present(rsize));
    }
    /* only show this net value if different from raw installed size */
    if (isize > 0 && rsize > 0) {
        status_text += fmt::format("Net Upgrade Size: {}\n", proper_present(isize - rsize));
    }

    res += '\n';
    return res;
}

}  // namespace

namespace alpm {

void setup_alpm(alpm_handle_t* handle) noexcept {
    parse_repos(handle);

    alpm_option_set_logcb(handle, cb_log, nullptr);
    alpm_option_set_eventcb(handle, cb_event, nullptr);
}

void destroy_alpm(alpm_handle_t* handle) noexcept {
    alpm_unregister_all_syncdbs(handle);
    alpm_release(handle);
}

void refresh_alpm(alpm_handle_t** handle, alpm_errno_t& err) noexcept {
    destroy_alpm(*handle);
    *handle = alpm_initialize("/", "/var/lib/pacman/", &err);
    setup_alpm(*handle);
}

auto display_targets(alpm_handle_t* handle, bool verbosepkglists, std::string& status_text) noexcept -> std::string {
    std::vector<pm_target_t> targets{};
    alpm_db_t* db_local = alpm_get_localdb(handle);

    for (alpm_list_t* i = alpm_trans_get_add(handle); i; i = alpm_list_next(i)) {
        auto* pkg = static_cast<alpm_pkg_t*>(i->data);
        pm_target_t targ{};
        targ.install     = pkg;
        targ.is_explicit = false;
        targ.remove      = alpm_db_get_pkg(db_local, alpm_pkg_get_name(pkg));
        targets.emplace_back(targ);
    }
    for (alpm_list_t* i = alpm_trans_get_remove(handle); i; i = alpm_list_next(i)) {
        auto* pkg = static_cast<alpm_pkg_t*>(i->data);
        pm_target_t targ{};
        targ.install     = nullptr;
        targ.is_explicit = false;
        targ.remove      = pkg;
        targets.emplace_back(targ);
    }
    return _display_targets(targets, verbosepkglists, status_text);
}

void add_targets_to_install(alpm_handle_t* handle, const std::vector<std::string>& targets) noexcept {
    /* Step 0: create a new transaction */
    if (alpm_trans_init(handle, ALPM_TRANS_FLAG_ALLDEPS | ALPM_TRANS_FLAG_ALLEXPLICIT | ALPM_TRANS_FLAG_NOLOCK) != 0) {
        spdlog::error("failed to create a new transaction ({})\n", alpm_strerror(alpm_errno(handle)));
        alpm_trans_release(handle);
        return;
    }

    /* Step 1: add targets to the created transaction */
    auto* dbs = alpm_get_syncdbs(handle);

    for (const auto& target : targets) {
        for (alpm_list_t* i = dbs; i != nullptr; i = i->next) {
            auto* db  = reinterpret_cast<alpm_db_t*>(i->data);
            auto* pkg = alpm_db_get_pkg(db, target.c_str());
            if (pkg) {
                if (alpm_add_pkg(handle, pkg) != 0) {
                    spdlog::error("failed to add package to be installed ({})\n", alpm_strerror(alpm_errno(handle)));
                }
                break;
            }
        }
    }
}

void add_targets_to_remove(alpm_handle_t* handle, const std::vector<std::string>& targets) noexcept {
    /* Step 0: create a new transaction */
    if (alpm_trans_init(handle, ALPM_TRANS_FLAG_ALLDEPS | ALPM_TRANS_FLAG_ALLEXPLICIT | ALPM_TRANS_FLAG_NOLOCK) != 0) {
        spdlog::error("failed to create a new transaction ({})\n", alpm_strerror(alpm_errno(handle)));
        alpm_trans_release(handle);
        return;
    }

    /* Step 1: add targets to the created transaction */
    auto* db_local = alpm_get_localdb(handle);

    for (const auto& target : targets) {
        auto* pkg = alpm_db_get_pkg(db_local, target.c_str());
        if (pkg) {
            if (alpm_remove_pkg(handle, pkg) != 0) {
                spdlog::error("failed to add package to be removed ({})\n", alpm_strerror(alpm_errno(handle)));
            }
        }
    }
}

void trans_init_error(alpm_handle_t* handle) noexcept {
    const auto err = alpm_errno(handle);
    spdlog::error("error: failed to init transaction ({})", alpm_strerror(err));
}

auto trans_release(alpm_handle_t* handle) noexcept -> std::int32_t {
    if (alpm_trans_release(handle) == -1) {
        spdlog::error("error: failed to release transaction ({})", alpm_strerror(alpm_errno(handle)));
        return -1;
    }
    return 0;
}

auto get_db(alpm_handle_t* handle, std::string_view dbname) noexcept -> alpm_db_t* {
    for (alpm_list_t* i = alpm_get_syncdbs(handle); i; i = i->next) {
        auto* db = static_cast<alpm_db_t*>(i->data);
        if (std::string_view{alpm_db_get_name(db)} == dbname) {
            return db;
        }
    }
    return nullptr;
}

bool process_pkg(alpm_handle_t* handle, alpm_pkg_t* pkg) noexcept {
    if (alpm_add_pkg(handle, pkg) == -1) {
        const auto err = alpm_errno(handle);
        spdlog::error("error: '{}': {}\n", alpm_pkg_get_name(pkg), alpm_strerror(err));
        return true;
    }
    return false;
}

bool group_exists(alpm_list_t* dbs, std::string_view group_name) noexcept {
    for (alpm_list_t* i = dbs; i; i = i->next) {
        auto* db = static_cast<alpm_db_t*>(i->data);
        if (alpm_db_get_group(db, group_name.data())) {
            return true;
        }
    }

    return false;
}

auto process_group(alpm_handle_t* handle, alpm_list_t* dbs, std::string_view group, std::int32_t error) noexcept -> std::int32_t {
    alpm_list_t* pkgs = alpm_find_group_pkgs(dbs, group.data());

    const auto count = alpm_list_count(pkgs);
    if (!count) {
        if (group_exists(dbs, group)) {
            return 0;
        }

        spdlog::error("error: target not found: {}", group);
        return 1;
    }

    std::int32_t ret = 0;
    if (error) {
        /* we already know another target errored. there is no reason to prompt the
         * user here; we already validated the group name so just move on since we
         * won't actually be installing anything anyway. */
        goto cleanup;
    }

    for (alpm_list_t* i = pkgs; i; i = alpm_list_next(i)) {
        auto* pkg = static_cast<alpm_pkg_t*>(i->data);

        if (process_pkg(handle, pkg) == 1) {
            ret = 1;
            goto cleanup;
        }
    }

cleanup:
    alpm_list_free(pkgs);
    return ret;
}

auto process_targname(alpm_handle_t* handle, alpm_list_t* dblist, std::string_view targname, std::int32_t error) noexcept -> std::int32_t {
    auto* pkg = alpm_find_dbs_satisfier(handle, dblist, targname.data());

    /* skip ignored packages when user says no */
    if (alpm_errno(handle) == ALPM_ERR_PKG_IGNORED) {
        spdlog::warn("skipping target: {}", targname);
        return 0;
    }

    if (pkg) {
        return process_pkg(handle, pkg);
    }
    /* fallback on group */
    return process_group(handle, dblist, targname, error);
}

auto process_target(alpm_handle_t* handle, std::string_view target, std::int32_t error) noexcept -> std::int32_t {
    /* process targets */
    std::int32_t ret{};

    // Find the first occurrence of '/'
    auto pos = target.find('/');
    if (pos != std::string_view::npos && pos != 0) {
        // Split target into database name and target name
        const auto dbname   = target.substr(0, pos);
        const auto targname = target.substr(pos + 1);

        auto* db = get_db(handle, dbname);
        if (!db) {
            spdlog::error("error: database not found: {}", dbname);
            return 1;
        }

        std::int32_t usage{};
        /* explicitly mark this repo as valid for installs since
         * a repo name was given with the target */
        alpm_db_get_usage(db, &usage);
        alpm_db_set_usage(db, usage | ALPM_DB_USAGE_INSTALL);

        auto* dblist = alpm_list_add(nullptr, db);
        ret          = process_targname(handle, dblist, targname, error);
        alpm_list_free(dblist);

        /* restore old usage, so we don't possibly disturb later
         * targets */
        alpm_db_set_usage(db, usage);
    } else {
        auto* dblist = alpm_get_syncdbs(handle);
        return process_targname(handle, dblist, target, error);
    }

    return ret;
}

auto check_syncdbs(alpm_handle_t* handle, size_t need_repos) noexcept -> std::int32_t {
    const auto* sync_dbs = alpm_get_syncdbs(handle);
    if (need_repos && sync_dbs == nullptr) {
        spdlog::error("error: no usable package repositories configured.");
        return 1;
    }
    return 0;
}

auto trans_init(alpm_handle_t* handle, std::int32_t flags) noexcept -> std::int32_t {
    if (check_syncdbs(handle, 0) == 1) {
        return -1;
    }

    if (alpm_trans_init(handle, flags) == -1) {
        trans_init_error(handle);
        return -1;
    }
    return 0;
}

void print_broken_dep(alpm_handle_t* handle, alpm_depmissing_t* miss) noexcept {
    const std::unique_ptr<char> depstring{alpm_dep_compute_string(miss->depend)};
    alpm_list_t* trans_add = alpm_trans_get_add(handle);
    if (miss->causingpkg == nullptr) {
        /* package being installed/upgraded has unresolved dependency */
        spdlog::warn("unable to satisfy dependency '{}' required by {}",
            depstring.get(), miss->target);
    } else if (auto* pkg = alpm_pkg_find(trans_add, miss->causingpkg)) {
        /* upgrading a package breaks a local dependency */
        spdlog::warn("installing {} ({}) breaks dependency '{}' required by {}",
            miss->causingpkg, alpm_pkg_get_version(pkg), depstring.get(), miss->target);
    } else {
        /* removing a package breaks a local dependency */
        spdlog::warn("removing {} breaks dependency '{}' required by {}",
            miss->causingpkg, depstring.get(), miss->target);
    }
}

auto sync_prepare_execute(alpm_handle_t* handle, std::string& conflict_msg) noexcept -> std::int32_t {
    alpm_list_t* packages = nullptr;
    alpm_list_t* data     = nullptr;
    std::int32_t retval{};

    /* Step 2: "compute" the transaction based on targets and flags */
    if (alpm_trans_prepare(handle, &data) == -1) {
        const auto err = alpm_errno(handle);
        spdlog::error("error: failed to prepare transaction ({})", alpm_strerror(err));
        switch (err) {
        case ALPM_ERR_PKG_INVALID_ARCH:
            for (alpm_list_t* i = data; i; i = alpm_list_next(i)) {
                const std::string_view pkg = static_cast<char*>(i->data);
                spdlog::info("package {} does not have a valid architecture", pkg);
            }
            break;
        case ALPM_ERR_UNSATISFIED_DEPS:
            for (alpm_list_t* i = data; i; i = alpm_list_next(i)) {
                auto* miss = static_cast<alpm_depmissing_t*>(i->data);
                print_broken_dep(handle, miss);
                alpm_depmissing_free(miss);
            }
            break;

        case ALPM_ERR_CONFLICTING_DEPS:
            for (alpm_list_t* i = data; i; i = alpm_list_next(i)) {
                auto* conflict                       = static_cast<alpm_conflict_t*>(i->data);
                const std::string_view pkg_conflict1 = alpm_pkg_get_name(conflict->package1);
                const std::string_view pkg_conflict2 = alpm_pkg_get_name(conflict->package2);

                /* only print reason if it contains new information */
                if (conflict->reason->mod == ALPM_DEP_MOD_ANY) {
                    conflict_msg += fmt::format("'{}' and '{}' are in conflict\n", pkg_conflict1, pkg_conflict2);
                    spdlog::info("'{}' and '{}' are in conflict", pkg_conflict1, pkg_conflict2);
                } else {
                    const std::unique_ptr<char> reason{alpm_dep_compute_string(conflict->reason)};
                    const std::string_view reason_str = reason ? reason.get() : "null";

                    conflict_msg += fmt::format("'{}' and '{}' are in conflict ({})\n", pkg_conflict1, pkg_conflict2, reason_str);
                    spdlog::info("'{}' and '{}' are in conflict ({})", pkg_conflict1, pkg_conflict2, reason_str);
                }
                alpm_conflict_free(conflict);
            }
            break;
        default:
            break;
        }
        retval = 1;
        goto cleanup;
    }

    packages = alpm_trans_get_add(handle);
    if (packages == nullptr) {
        /* nothing to do: just exit without complaining */
        goto cleanup;
    }

    /* Step 4: release transaction resources */
cleanup:
    alpm_list_free(data);
    if (trans_release(handle) == -1) {
        retval = 1;
    }

    return retval;
}

auto sync_trans(alpm_handle_t* handle, const std::vector<std::string>& targets, std::int32_t flags, std::string& conflict_msg) noexcept -> std::int32_t {
    /* Step 1: create a new transaction... */
    if (trans_init(handle, flags) == -1) {
        return 1;
    }

    /* process targets */
    std::int32_t retval{};
    for (auto&& targ : targets) {
        if (process_target(handle, targ, retval) == 1) {
            retval = 1;
        }
    }

    if (retval) {
        trans_release(handle);
        return retval;
    }

    return sync_prepare_execute(handle, conflict_msg);
}

auto get_package_view(alpm_handle_t* handle, std::string_view pkgname) noexcept -> std::optional<PackageView> {
    auto* dbs = alpm_get_syncdbs(handle);
    for (alpm_list_t* i = dbs; i != nullptr; i = i->next) {
        auto* db  = reinterpret_cast<alpm_db_t*>(i->data);
        auto* pkg = alpm_db_get_pkg(db, pkgname.data());
        if (pkg == nullptr) {
            continue;
        }
        return std::make_optional<PackageView>(PackageView{.pkgver = alpm_pkg_get_version(pkg), .desc = alpm_pkg_get_desc(pkg)});
    }
    return std::nullopt;
}

}  // namespace alpm
