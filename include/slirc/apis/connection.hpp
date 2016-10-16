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

#ifndef SLIRC_APIS_CONNECTION_HPP_INCLUDED
#define SLIRC_APIS_CONNECTION_HPP_INCLUDED

#include "../detail/system.hpp"

#include "../event.hpp"
#include "../module.hpp"

namespace slirc {

class irc;

namespace apis {

/** \brief Handles a connection to an IRC server.
 *
 * \note All functions in this API are thread safe and can be called from
 *       threads other than the
 */
struct SLIRCAPI connection: module<connection> {
	using module::module;

	/// Describes the state of the connection.
	enum class state: ::slirc::event::underlying_id_type {
		/** \brief The connection is disconnected and inactive.
		 *
		 * This will be raised as an event when the connection was terminated.
		 */
		disconnected,

		/** \brief The connection is in the process of being established.
		 *
		 * This will be raised as an event the connecting process begins.
		 */
		connecting,

		/** \brief The connection is established.
		 *
		 * This will be raised as an event when the connecting process has
		 * finished successfully.
		 */
		connected,

		/** \brief The connection is about to be disconnected.
		 *
		 * This will be raised as an event when the connection has been
		 * requested to terminate.
		 *
		 * \note This will only be used as an event and never be reported back
		 *       from \c current_status().
		 */
		disconnecting,

		/** \brief The connection status has changed.
		 *
		 * This will be raised as an event whenever the connection status
		 * changes. Any event of the other ids will be handled as this id
		 * before their specific id.
		 *
		 * \note This will only be used as an event and never be reported back
		 *       from \c current_status().
		 *
		 * \note When receiving this event, you can check the original event id
		 *       to find out the state changed to. Calling current_state() may
		 *       yield the wrong result, as events are handled asynchronously!
		 */
		changed
	};

	/** Additional events.
	 */
	enum events : ::slirc::event::underlying_id_type {
		/**
		 * Raised when a line is received from the IRC server.
		 *
		 * Contains a \c received_data component.
		 */
		received_line
	};

	/** Contains the data received from the connection.
	 *
	 * Is attached to a \c received_line event.
	 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
	struct received_data: component<received_data> {
		/** The line of data received by the connection.
		 *
		 * Leading whitespace and the line break are removed. Whitespace
		 * between arguments is not normalized.
		 */
		std::string data;
	};
#pragma GCC diagnostic pop

	/** \brief Connects to the IRC server.
	 *
	 * \throw slirc::exceptions::already_connected if the connection is not
	 *        currently disconnected.
	 */
	virtual void connect() = 0;

	/** \brief Disconnects from the IRC server.
	 *
	 * If the connection is already disconnected, this will be a noop. No
	 * exception shall be thrown in this case.
	 */
	virtual void disconnect() = 0;

	/** \brief Returns the current state of the connection.
	 *
	 * \return
	 *     - \c state::disconnected if the connection is disconnected and inactive,
	 *     - \c state::connecting if the connection is in the progress of being established,
	 *     - \c state::connected if the connection has been established
	 *
	 * \note \c state::disconnecting and \c state::changed are only used as
	 *       events and will never be returned by this function.
	 *
	 * \note As events are handled asynchronously, when calling this function
	 *       from an event handler handling one of the state events, it may
	 *       return a different state than the event has;
	 *       e.g. an event handler receiving a \c state::connecting event may
	 *       find the return value of this function to be \c state::connected.
	 */
	virtual state current_state() = 0;

	/** \brief Sends data to the server.
	 *
	 * If the connection is established, the data passed is added to the send
	 * queue and sent at the next possible opportunity.
	 *
	 * \param data A pointer to the data to send.
	 * \param length The length of the data.
	 *
	 * \note The passed data will be appended to the send queue.
	 */
	inline void send_raw(const char *data, std::size_t length) {
		do_send_raw(data, length);
	}

	/** \brief Sends data to the server.
	 *
	 * If the connection is established, the data passed is added to the send
	 * queue and sent at the next possible opportunity.
	 *
	 * \param data A pointer to the null terminated data to send.
	 *
	 * \note The passed data will be appended to the send queue.
	 */
	inline void send_raw(const char *data) {
		unsigned length=0;
		while(data[length]) ++length;
		do_send_raw(data, length);
	}

	/** \brief Sends data to the server.
	 *
	 * If the connection is established, the data passed is added to the send
	 * queue and sent at the next possible opportunity.
	 *
	 * \tparam N The length of the array.
	 *
	 * \param data An array containing the data to send; trailing <tt>'\0'</tt>
	 *             are truncated.
	 *
	 * \note The passed data will be appended to the send queue.
	 */
	template<std::size_t N>
	inline void send_raw(const char (&data)[N]) {
		std::size_t length=N;
		while(length && !data[length-1]) --length;
		do_send_raw(data, length);
	}

protected:
	/** \brief Sends data to the server.
	 *
	 * If the connection is established, the data passed is added to the send
	 * queue and sent at the next possible opportunity.
	 *
	 * \param data A pointer to the data to send.
	 * \param length The length of the data.
	 *
	 * \note The passed data will be appended to the send queue.
	 */
	virtual void do_send_raw(const char *data, std::size_t length) = 0;
};

SLIRC_REGISTER_EVENT_ID_ENUM(connection::state);
SLIRC_REGISTER_EVENT_ID_ENUM(connection::events);

}
}

#ifndef SLIRC_NO_INCLUDE_API_DEFAULT_IMPLEMENTATION
#	include "../modules/connection.hpp"
#endif

#endif // SLIRC_APIS_CONNECTION_HPP_INCLUDED
