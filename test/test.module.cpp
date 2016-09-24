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

#include "testcase.hpp"

#define SLIRC_IRC_HPP_INCLUDED
namespace slirc {
namespace test { struct test_overrides {
};}

struct irc {
};
}

#include "../include/slirc/module.hpp"

struct some_module: slirc::module<some_module> {
	some_module(slirc::irc &irc, slirc::irc *&pirc)
	: slirc::module<some_module>(irc) {
		SLIRC_ASSERT(false && "this tests the assertion catching");
		pirc = &(this->irc);
	}
};

TEST_CASE("module - irc member gets set correctly", "") {
	slirc::irc irc;
	slirc::irc *pirc = nullptr;

	REQUIRE(pirc == &irc);
}
