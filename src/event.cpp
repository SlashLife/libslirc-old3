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

#include "../include/slirc/event.hpp"

#include "../include/slirc/irc.hpp"

SLIRCAPI slirc::event::event(constructor_tag, slirc::irc &irc_, id_type original_id_)
: irc(irc_)
, original_id(original_id_)
, current_id(current_id_mutable)
, current_id_mutable(original_id_)
, next_id_index(0) {
	if (!original_id_) {
		throw exceptions::invalid_event_id();
	}
	queued_ids.push_back(original_id_);
}

SLIRCAPI void slirc::event::handle();
SLIRCAPI void slirc::event::handle_as(id_type id);

SLIRCAPI slirc::event::queuing_result slirc::event::queue_as(
	id_type id, queuing_strategy strategy, queuing_position position
);

SLIRCAPI bool slirc::event::unqueue(id_type id);
SLIRCAPI bool slirc::event::unqueue(id_type::matcher matcher);

SLIRCAPI bool slirc::event::is_queued_as(id_type id);
SLIRCAPI bool slirc::event::is_queued_as(id_type::matcher matcher);

SLIRCAPI slirc::event::id_type slirc::event::pop_next_queued_id();
