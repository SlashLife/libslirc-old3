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

#include "../include/slirc/event.hpp"
#include "../include/slirc/irc.hpp"
#include "../include/slirc/apis/event_queue.hpp"

enum class test_events: slirc::event::underlying_id_type {

};
SLIRC_REGISTER_EVENT_ID_ENUM(class test_events);

TEST_CASE("apis/event_queue - connections") {
	GIVEN("an IRC context with an event queue loaded") {
		slirc::irc irc;
		irc.unload<slirc::apis::event_queue>();
		irc.load<test_event_queue>();


	}
}
