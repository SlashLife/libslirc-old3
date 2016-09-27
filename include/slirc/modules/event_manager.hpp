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

	/** \brief Connects an event handler to an event id.
	 *
	 * \param event_id The event_id to connect to.
	 * \param handler The handler to connect.
	 * \param priority The priority to connect the handler with.
	 *
	 * \return A connection representing the added handler.
	 */
	virtual connection connect(event::id_type event_id, handler_type handler, connection_priority priority = normal) override;

	/** \brief Handles an event.
	 *
	 * Handles the event for all event ids that it is queued up for.
	 *
	 * Right before returning, all events added for immediate handling using
	 * event::afterwards() will be queued <b>to the front</b> of the event queue.
	 *
	 * \param e The event to handle.
	 *
	 * \note To handle an event, <tt>e->handle()</tt> is preferrable to invoking
	 *       this function directly!
	 */
	virtual void handle(event::pointer e) override;

	/** \brief Handles an event.
	 *
	 * Handles the event for its \c current_id
	 *
	 * \param e The event to handle.
	 *
	 * \note To handle an event for a specific event id,
	 *       <tt>e->handle_as(event_id)</tt> is preferrable to invoking
	 *       this function directly!
	 */
	virtual void handle_as(event::pointer e) override;



	/** \brief Queues an event.
	 *
	 * Appends an event to the queue.
	 *
	 * \param e The event to append. Must not be a \c nullptr.
	 *
	 * \note This function is thread safe.
	 */
	virtual void queue(event::pointer e) override;

	/** \brief Wait for an event.
	 *
	 * Waits for an event to become available on the queue and returns it.
	 *
	 * \return An event from the queue.
	 *
	 * \note While this function will wait indeterminately long for an event to
	 *       be returned, it \em may return a \c nullptr if the module is being
	 *       destructed.
	 * \note This function is thread safe.
	 */
	virtual event::pointer wait_event() override;

	/** \brief Wait for an event.
	 *
	 * Waits for an event to become available on the queue and returns it.
	 *
	 * \param timeout The maximal time to wait for an event.
	 *
	 * \return An event from the queue or \c nullptr if a timeout occurred.
	 *
	 * \note This function may return early spuriously.
	 * \note This function is thread safe.
	 */
	virtual event::pointer wait_event(std::chrono::milliseconds timeout) override;

	/** \brief Wait for an event.
	 *
	 * Registers an event consumer to wait for an event.
	 *
	 * The order in which multiple event consumers and calls to wait_event()
	 * waiting on the same queue are satisfied is unspecified. However, each
	 * event consumer is eventually guaranteed to be called exactly once.
	 *
	 * \param callback The event consumer to be registered.
	 *
	 * \note This function is thread safe.
	 * \note In case of the destruction of the queue, the consumer may be
	 *       called with a nullptr.
	 */
	virtual void wait_event(event_consumer_type callback) override;

protected:
	/** \brief Used to order event handler connections.
	 *
	 * \param lhs left hand side parameter
	 * \param rhs right hand side parameter
	 *
	 * \return whether \c lhs is ordered before \c rhs
	 */
	virtual bool connection_less(const disconnector_type &lhs, const disconnector_type &rhs) override;

private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}
}

#endif // SLIRC_MODULES_EVENT_MANAGER_HPP_INCLUDED
