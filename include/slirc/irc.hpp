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

#ifndef SLIRC_IRC_HPP_INCLUDED
#define SLIRC_IRC_HPP_INCLUDED

#include "detail/system.hpp"

#include <map>
#include <memory>
#include <type_traits>

#include <boost/utility.hpp>

#include "component_container.hpp"
#include "event.hpp"

namespace slirc {

namespace apis {
	struct event_queue;
}

/**
 */
class SLIRCAPI irc: public takes_components, private boost::noncopyable {
	typedef std::map<std::type_index, >
public:
	irc();



	// Module API

	template<typename Module, typename... Args>
	Module &load(Args... && args); // TODO: impl

	template<typename Module>
	Module &unload(); // TODO: impl

	template<typename Module>
	Module &get(); // TODO: impl



	// event api

	inline apis::event_queue &event_queue() const {
		SLIRC_ASSERT( event_queue_ &&
			"IRC context should never be without a loaded event queue module!" );
		return *event_queue_;
	}

	inline event::pointer make_event(event::id_type id) {
		return event::make_event(*this, id);
	}

private:
	apis::event_queue *event_queue_;
};

}

#endif // SLIRC_IRC_HPP_INCLUDED
