/*
    SPDX-FileCopyrightText: 2020-2022 Mladen Milinkovic <max@smoothware.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef HELPERS_COMMON_H
#define HELPERS_COMMON_H

#include <type_traits>

#define $(str) QStringLiteral(str)

#define GLUE(x, y) x y // needed to make MSVC happy

#define RE$1(regExp) QRegularExpression(QStringLiteral(regExp))
#define RE$2(regExp, opts) QRegularExpression(QStringLiteral(regExp), opts)
#define RE$_VA(_1, _2, NAME, ...) NAME
#define RE$(...) GLUE(RE$_VA(__VA_ARGS__, RE$2, RE$1), (__VA_ARGS__))

#define staticRE$2(sVar, regExp) const static QRegularExpression sVar(QStringLiteral(regExp))
#define staticRE$3(sVar, regExp, opts) const static QRegularExpression sVar(QStringLiteral(regExp), opts)
#define staticRE$_VA(_1, _2, _3, NAME, ...) NAME
#define staticRE$(...) GLUE(staticRE$_VA(__VA_ARGS__, staticRE$3, staticRE$2), (__VA_ARGS__))

#define REu QRegularExpression::UseUnicodePropertiesOption
#define REm QRegularExpression::MultilineOption
#define REs QRegularExpression::DotMatchesEverythingOption
#define REi QRegularExpression::CaseInsensitiveOption

/**
 * @brief Return number of set bits in value - using parallel bit count
 * https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
 * @param v unsigned value to count bits
 * @return number of set bits
 */
template<typename T, class = typename std::enable_if<std::is_unsigned<T>::value>::type>
int bitCount(T v)
{
	const constexpr T x55 = ~T(0) / T(3);
	const constexpr T x33 = ~T(0) / T(5);
	const constexpr T x0f = ~T(0) / T(255) * T(15);
	const constexpr T x01 = ~T(0) / T(255);
	v = v - ((v >> 1) & x55);
	v = (v & x33) + ((v >> 2) & x33);
	v = (v + (v >> 4)) & x0f;
	return (v * x01) >> (sizeof(T) - 1) * 8;
}

#endif // HELPERS_COMMON_H
