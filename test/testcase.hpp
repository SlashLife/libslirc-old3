/***************************************************************************
**  Copyright 2016-2016 by Simon "SlashLife" Stienen                      **
**  http://projects.slashlife.org/libslirc/                               **
**  libslirc@projects.slashlife.org                                       **
**                                                                        **
**  This file is part of libslIRC.                                        **
**                                                                        **
**  libslIRC is free software: you can redistribute it and/or modify      **
**  it under the terms of the GNU Lesser General Public License as        **
**  published by the Free Software Foundation, either version 3 of the    **
**  License, or (at your option) any later version.                       **
**                                                                        **
**  libslIRC is distributed in the hope that it will be useful,           **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  and the GNU Lesser General Public License along with libslIRC.        **
**  If not, see <http://www.gnu.org/licenses/>.                           **
***************************************************************************/

#pragma once

#ifndef SLIRC_TEST_TESTCASE_HPP_INCLUDED
#define SLIRC_TEST_TESTCASE_HPP_INCLUDED

#include <sstream>
#include <stdexcept>
#include <string>

#define SLIRC_TESTCASE
#define SLIRC_EXPORTS

namespace slirc { namespace test {
	struct assertion_failed_exception: std::exception {
		const char *file;
		unsigned line;
		const char *cond;

		assertion_failed_exception(const char *file, unsigned line, const char *cond)
		: file(file)
		, line(line)
		, cond(cond) {}
		assertion_failed_exception(const assertion_failed_exception &)=default;
		assertion_failed_exception &operator=(const assertion_failed_exception &)=default;

		const char *what() const noexcept {
			static std::string msg;
			std::stringstream ss;
			ss
				<< "assertion failed: "
				<< file << '(' << line << "): "
				<< cond;
			msg = ss.str();
			return msg.c_str();
		}
	};
	inline void assert_(bool cond, const char *file, unsigned line, const char *text) {
		if (!(cond))
			throw ::slirc::test::assertion_failed_exception(file, line, text);
	}
}}

// override internal assert to make them testable -
// test code should not invoke assertions unintentionally anyway.
#undef SLIRC_ASSERT
#define SLIRC_ASSERT(cond) ::slirc::test::assert_(cond, __FILE__, __LINE__, #cond)

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch/include/catch.hpp"

#define REQUIRE_ASSERTION_FAILURE(cond) \
	REQUIRE_THROWS_AS(cond, ::slirc::test::assertion_failed_exception)

#endif // SLIRC_TEST_TESTCASE_HPP_INCLUDED
