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

#ifndef SLIRC_APIS_EVENT_MANAGER_HPP_INCLUDED
#define SLIRC_APIS_EVENT_MANAGER_HPP_INCLUDED

#include "../detail/system.hpp"

#include <chrono>
#include <functional>
#include <vector>

#include "../component.hpp"
#include "../event.hpp"
#include "../module.hpp"

namespace slirc {
namespace apis {

/** \brief Defines the interface for the main event manager.
 */
struct SLIRCAPI event_manager: module<event_manager> {
	friend class ::slirc::event;
protected:
	/// \brief Signature for disconnectors.
	typedef std::function<void(event_manager*)> disconnector_type;

	/** \brief Holds additional follow up events to this event.
	 *
	 * Added to events on call of event::afterwards()
	 */
	struct handle_afterwards: component<handle_afterwards> {
		/// \brief Contains the events.
		std::vector<event::pointer> events;
	};

public:
	/** \brief Represents the connection of an event handler.
	 */
	struct connection {
	private:
		disconnector_type disconnector;
		event_manager *emgr;

		connection(disconnector_type discon, event_manager *manager)
		: disconnector(discon)
		, emgr(manager) {}

	public:
		friend struct ::std::less<connection>;
		friend struct ::slirc::apis::event_manager;

		/** \brief Creates a connection object not associated with any event
		 *         handler.
		 */
		connection()
		: disconnector()
		, emgr(nullptr) {}

		/** \brief Copies a connection.
		 */
		connection(const connection &)=default;

		/** \brief Assigns a different connection.
		 */
		connection &operator=(const connection &)=default;

		/** \brief Get the event manager this connection is attached to.
		 *
		 * \return A pointer to the event manager this connection is attached to
		 *         or nullptr if it is not attached to one.
		 */
		inline event_manager *get_manager() const {
			return emgr;
		}

		/** \brief Checks whether the associated event handler is still connected.
		 *
		 * \return
		 *     - \c true if the handler is still connected,
		 *     - \c false if it has been disconnected
		 *
		 * \note If the connection represented by this instance has been
		 *       disconnected through another instance, the behavior is
		 *       undefined.
		 */
		inline bool connected() const {
			return emgr;
		}

		/** \brief Disconnects the associated event handler.
		 *
		 * Disconnects the associated event handler and disassociates this
		 * connection with it. If this instance is not associated with any event
		 * handler, nothing will happen.
		 *
		 * This function can be called at any any time while the associated
		 * event manager is fully constructed.
		 *
		 * \note If the connection represented by this instance has been
		 *       disconnected through another instance, the behavior is
		 *       undefined.
		 */
		inline void disconnect() {
			if (emgr) {
				disconnector(emgr);
				emgr = nullptr;
				disconnector = disconnector_type();
			}
		}

		/** \brief Checks whether two connections represent different event handlers.
		 *
		 * \param other The connection to compare against.
		 *
		 * \return
		 *     - \c true if the other connection represents a different event handler,
		 *     - \c false if both connections represent the same event handler
		 *
		 * \note Two unassociated connections are considered equal.
		 */
		inline bool operator!=(const connection &other) const {
			return (emgr != other.emgr) || (emgr && (
				emgr->connection_less(disconnector, other.disconnector) ||
				emgr->connection_less(other.disconnector, disconnector)
			));
		}

		/** \brief Checks whether two connections represent the same event handler.
		 *
		 * \param other The connection to compare against.
		 *
		 * \return
		 *     - \c true if both connections represent the same event handler,
		 *     - \c false if the other connection represents a different event handler
		 *
		 * \note Two unassociated connections are considered equal.
		 */
		inline bool operator==(const connection &other) const {
			return !(*this != other);
		}
	};
	friend struct ::std::less<connection>;

	/// \brief Event types related to handling events.
	enum class events: event::underlying_id_type {
		/// executed right before handling of an event begins
		begin_handling,

		/// executed right before finishing the handling of an event
		/// \note if more event ids are queued during the handling of
		///       this event id, they will be handled and afterwards
		///       \b another \a finishing_handling id will be handled.
		finishing_handling,

		/// executed after finishing handling the event; event ids
		/// queued during this event id will stay in the queue and will not
		/// be handled during this call
		/// \note this is the last chance to add events to be queued up
		///       next using events::afterwards()
		finished_handling
	};

	/// \brief The signature definition for event handlers
	typedef std::function<void(event::pointer)> handler_type;

	/** \brief The signature for event consumers.
	 *
	 * An event consumer is called when an event becomes available on a queue
	 * and returns whether or not it will accept the event.
	 *
	 * \return
	 *     - \c true if the consumer accepts and will handle the event posted to it,
	 *     - \c false otherwise
	 *
	 * \note Event consumers must be thread safe and safe to call at any time,
	 *       even when they are no longer interested in events.
	 */
	typedef std::function<bool(event::pointer)> event_consumer_type;

	using module::module;

	/// \brief The priority how early or late within the same event id a handler should be called.
	enum connection_priority: int {
		first     = -1000, ///< The connected handler must be called before everything else \note As opposed to all other priorities, event handlers added with this priority will be added in a \b last come, first serve manner!
		filter    =  -800, ///< The connected handler needs to filter or change the event before it is handled
		highest   =  -600, ///< This handler needs to be run before normal subscribers
		higher    =  -400, ///< This handler needs to be run before normal subscribers
		high      =  -200, ///< This handler needs to be run before normal subscribers
		normal    =     0, ///< This handler is a normal subscriber
		low       =   200, ///< This handler needs to be run after normal subscribers
		lower     =   400, ///< This handler needs to be run after normal subscribers
		lowest    =   600, ///< This handler needs to be run after normal subscribers
		summarize =   800, ///< This handler will act on the results of this event after the other handlers have run
		last      =  1000  ///< This handler must be called after all other handlers have finished
	};

	/** \brief Connects an event handler to an event id.
	 *
	 * \param event_id The event_id to connect to.
	 * \param handler The handler to connect.
	 * \param priority The priority to connect the handler with.
	 *
	 * \return A connection representing the added handler.
	 */
	virtual connection connect(event::id_type event_id, handler_type handler, connection_priority priority = normal) = 0;



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
	virtual void handle(event::pointer e) = 0;

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
	virtual void handle_as(event::pointer e) = 0;



	/** \brief Queues an event.
	 *
	 * Appends an event to the queue.
	 *
	 * \param e The event to append. Must not be a \c nullptr.
	 *
	 * \note This function is thread safe.
	 */
	virtual void queue(event::pointer e) = 0;

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
	virtual event::pointer wait_event() = 0;

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
	virtual event::pointer wait_event(std::chrono::milliseconds timeout) = 0;

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
	virtual void wait_event(event_consumer_type callback) = 0;

protected:
	/** \brief Initializes a connection for an event handler.
	 *
	 * \param disconnector A callback to disconnect the event handler.
	 *
	 * \return A connection suitable to disconnect the event handler.
	 */
	inline connection make_connection(const disconnector_type &disconnector) const {
		return connection(disconnector, const_cast<event_manager*>(this));
	}

	/** \brief Used to order event handler connections.
	 *
	 * \param lhs left hand side parameter
	 * \param rhs right hand side parameter
	 *
	 * \return whether \c lhs is ordered before \c rhs
	 *
	 * \note It is guaranteed that \c lhs and \c rhs are disconnectors belonging
	 *       to the instance that this function is called on.
	 */
	virtual bool connection_less(const disconnector_type &lhs, const disconnector_type &rhs) = 0;
};

SLIRC_REGISTER_EVENT_ID_ENUM(event_manager::events);

}
}

namespace std {
	/// \brief Used to order event handler connections.
	template<>
	struct less<::slirc::apis::event_manager::connection> {
		/** \brief Used to order event handler connections.
		 *
		 * \param lhs left hand side parameter
		 * \param rhs right hand side parameter
		 *
		 * \return
		 *     - \c true if \c lhs is ordered before \c rhs,
		 *     - \c false otherwise
		 */
		bool operator()(
			const ::slirc::apis::event_manager::connection &lhs,
			const ::slirc::apis::event_manager::connection &rhs
		) const {
			return (lhs.emgr != rhs.emgr)
				? std::less<decltype(lhs.emgr)>()(lhs.emgr, rhs.emgr)
				: (lhs.emgr && lhs.emgr->connection_less(lhs.disconnector, rhs.disconnector));
		}
	};
}

#ifndef SLIRC_NO_INCLUDE_API_DEFAULT_IMPLEMENTATION
#	include "../modules/event_manager.hpp"
#endif

#endif // SLIRC_APIS_EVENT_MANAGER_HPP_INCLUDED
