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

#ifndef SLIRC_MODULES_EVENT_MANAGER_HPP_INCLUDED
#define SLIRC_MODULES_EVENT_MANAGER_HPP_INCLUDED

#include "../detail/system.hpp"

#include "../apis/event_manager.hpp"

namespace slirc {

class irc;

namespace modules {

/** \brief The default implementation for the main event manager interface.
 */
struct SLIRCAPI event_manager: apis::event_manager {
	/** \brief Constructs an event manager
	 *
	 * \param irc_ The IRC context to load this module into.
	 */
	event_manager(slirc::irc &irc_);

	virtual connection connect(event::id_type event_id, handler_type handler, connection_priority priority = normal) override;
	virtual void handle(event::pointer e) override;
	virtual void handle_as(event::pointer e) override;

	virtual void queue(event::pointer e) override;
	virtual event::pointer wait_event() override;
	virtual event::pointer wait_event(std::chrono::milliseconds timeout) override;
	virtual void wait_event(event_consumer_type callback) override;

protected:
	virtual bool connection_less(const disconnector_type &lhs, const disconnector_type &rhs) override;

private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}
}

#endif // SLIRC_MODULES_EVENT_MANAGER_HPP_INCLUDED
