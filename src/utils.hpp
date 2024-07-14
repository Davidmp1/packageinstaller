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

#ifndef UTILS_HPP
#define UTILS_HPP

#include "versionnumber.hpp"

#include <algorithm>    // for transform, for_each
#include <ranges>       // for ranges::*
#include <string_view>  // for string_view
#include <vector>       // for vector

#if 0
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <QString>
#include <QStringList>

template <typename T>
struct fmt::formatter<T, std::enable_if_t<std::is_base_of<QString, T>::value, char>> : fmt::formatter<std::string> {
    template <typename FormatCtx>
    auto format(const QString& str, FormatCtx& ctx) {
        return fmt::formatter<std::string>::format(str.toStdString(), ctx);
    }
};

namespace fmt {

template <>
struct formatter<std::pair<const QString, VersionNumber>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::pair<const QString, VersionNumber>& fp, FormatContext& ctx) {
        return format_to(ctx.out(), "\n{}{} {}{}", "{", fp.first, fp.second, "}");
    }
};

template <>
struct formatter<std::unordered_map<QString, VersionNumber>> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const std::unordered_map<QString, VersionNumber>& fm, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", fmt::join(fm.begin(), fm.end(), ""));
    }
};

template <>
struct formatter<QStringList> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const QStringList& fm, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", fmt::join(fm, ", "));
    }
};
}  // namespace fmt
#endif

namespace utils {

/// @brief Make a split view from a string into multiple lines based on a delimiter.
/// @param str The string to split.
/// @param delim The delimiter to split the string.
/// @return A range view representing the split lines.
constexpr auto make_split_view(std::string_view str, char delim = '\n') noexcept {
    constexpr auto functor = [](auto&& rng) {
        return std::string_view(&*rng.begin(), static_cast<size_t>(std::ranges::distance(rng)));
    };
    constexpr auto second = [](auto&& rng) { return rng != ""; };

    return str
        | std::ranges::views::split(delim)
        | std::ranges::views::transform(functor)
        | std::ranges::views::filter(second);
}

/// @brief Split a string into multiple lines based on a delimiter.
/// @param str The string to split.
/// @param delim The delimiter to split the string.
/// @return A vector of strings representing the split lines.
constexpr auto make_multiline(std::string_view str, char delim = '\n') noexcept -> std::vector<std::string> {
    return [&]() constexpr {
        std::vector<std::string> lines{};
        std::ranges::for_each(utils::make_split_view(str, delim), [&](auto&& rng) { lines.emplace_back(rng); });
        return lines;
    }();
}

/// @brief Join a vector of strings into a single string using a delimiter.
/// @param lines The lines to join.
/// @param delim The delimiter to join the lines.
/// @return The joined lines as a single string.
constexpr auto join(const std::vector<std::string_view>& lines, char delim = '\n') noexcept -> std::string {
    return lines | std::ranges::views::join_with(delim) | std::ranges::to<std::string>();
}

/// @brief Join a range into a single string using a delimiter.
/// @param first The range begin.
/// @param last The range end.
/// @param delim The delimiter to join the range.
/// @return The joined range as a single string.
template <std::input_iterator I, std::sentinel_for<I> S>
constexpr auto join_range(I first, S last, char delim = '\n') noexcept -> std::string {
    return std::ranges::subrange(first, last) | std::ranges::views::join_with(delim) | std::ranges::to<std::string>();
}

}  // namespace utils

#endif  // UTILS_HPP
