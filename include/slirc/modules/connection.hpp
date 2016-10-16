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

#ifndef SLIRC_MODULES_CONNECTION_HPP_INCLUDED
#define SLIRC_MODULES_CONNECTION_HPP_INCLUDED

#include "../detail/system.hpp"

#include <memory>
#include <string>

#include "../component.hpp"
#include "../apis/connection.hpp"

namespace boost { namespace system {
	struct error_code;
}}

namespace slirc {

class irc;

namespace modules {

/** \brief Default implementation for the IRC connection.
 */
struct SLIRCAPI connection: apis::connection {
private:
	struct impl;
	std::shared_ptr<impl> impl_;

public:
	/** \brief Defines addtional events.
	 */
	enum events : event::underlying_id_type {
		/**
		 * Raised when an error occurs. Contains an error_info component.
		 */
		error
	};

	/** \brief Contains additional error information.
	 *
	 * Contains additional error information; is attached to an events::error
	 * event.
	 */
	struct error_info: component<error_info> {
		friend struct connection::impl;

		/** \brief A string representation of the error.
		 *
		 * \return A string containing the error message.
		 */
		const std::string &message() const;

		/** \brief A string representation of the error.
		 *
		 * \return A string containing the error message.
		 */
		const boost::system::error_code &error_code() const;

	private:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
		error_info() = default;
#pragma GCC diagnostic pop

		struct impl;
		std::unique_ptr<impl> impl_;
	};

	/** \brief Constructs an IRC connection object.
	 * Constructs an IRC connection object without setting an endpoint.
	 *
	 * \param irc The IRC context this connection object belongs to.
	 *
	 * \note The constructor will not initialize the connecting process. You
	 *       will need to call \c connect() manually. This is to allow for a
	 *       relaxed loading order of network connection dependent modules.
	 */
	connection(slirc::irc &irc);

	/** \brief Constructs an IRC connection object.
	 * Constructs an IRC connection object and sets the endpoint.
	 *
	 * Equivalent to constructing without an endpoint and subsequently calling
	 * <tt>set_endpoint(endpoint, port)</tt>.
	 *
	 * \param irc The IRC context this connection object belongs to.
	 * \param endpoint The endpoint passed to \c set_endpoint()
	 * \param port The port passed to \c set_endpoint()
	 *
	 * \note The constructor will not initialize the connecting process. You
	 *       will need to call \c connect() manually. This is to allow for a
	 *       relaxed loading order of network connection dependent modules.
	 */
	connection(slirc::irc &irc, const std::string &endpoint, unsigned port=0)
	: connection(irc) {
		set_endpoint(endpoint, port);
	}

	/** \brief Destructs the IRC connection object.
	 *
	 * Terminates the connection (if any) and destructs the IRC connection
	 * object.
	 *
	 * \note As opposed to \c disconnect(), this will block until the connection
	 *       is actually terminated.
	 */
	~connection();

	/** \brief Sets the endpoint.
	 *
	 * Sets the endpoint.
	 *
	 * The format for the endpoint can be a server name or IP address,
	 * optionally followed by a port number, separated by <tt>':'</tt>. If you
	 * want to specify a port number for an IPv6 address, the IPv6 address must
	 * be enclodes in square brackets. For example:
	 * - <tt>irc.freenode.net</tt>
	 * - <tt>irc.freenode.net:8000</tt>
	 * - <tt>127.0.0.1</tt>
	 * - <tt>127.0.0.1:8000</tt>
	 * - <tt>::1</tt>
	 * - <tt>[::1]</tt>
	 * - <tt>[::1]:8000</tt>
	 *
	 * Alternatively you can use a URL of the format
	 * <tt>"protocol://[user[:pass]@]server[:port][/path][?query][#fragment]"</tt>.
	 *
	 * \c protocol can be any of \c irc, \c ircs, \c tcp or \c ssl. \c irc and
	 * \c tcp are equivalent and specify an endpoint \em without SSL, \c ircs
	 * and \c ssl are equivalent and specify an endpoint \em with SSL.
	 *
	 * \c server and port represent the server and port as in the previous
	 * example.
	 *
	 * All the optional parts will be ignored by this class (but can be parsed
	 * out by the user to set up other modules for purposes like authentication
	 * or automatically joining channels).
	 *
	 * For example (ignored optional parts are left out):
	 * - <tt>irc://irc.freenode.net</tt>
	 * - <tt>tcp://irc.freenode.net:8000</tt>
	 * - <tt>ircs://irc.freenode.net</tt>
	 * - <tt>ssl://irc.freenode.net:7000</tt>
	 *
	 * The port to be used is determined as follows:
	 * - If the parameter \c port is not \c 0, it will be used.
	 * - Otherwise if \c endpoint contains a port number, it will be used.
	 * - Otherwise the port number will default to \c 6667 for endpoints
	 *   \em without SSL and \c 6697 for endpoints \em with SSL.
	 *
	 * \param endpoint The endpoint for the connection.
	 * \param port The port for the connection.
	 *
	 * \throw slirc::exceptions::already_connected if the connections state
	 *        is not state::disconnected
	 *
	 * \note When using the URL form, no URL decoding of the used fields will
	 *       take place!
	 * \note The protocols <tt>ircs</tt> and <tt>ssl</tt> are not available if
	 *       libslirc was built without SSL support.
	 */
	void set_endpoint(const std::string &endpoint, unsigned port=0);

	/** \brief Sets the endpoint and connects to it.
	 *
	 * Equivalent to a call to <tt>set_endpoint(endpoint, port)</tt> followed by
	 * a call to <tt>connect()</tt>.
	 *
	 * \param endpoint The endpoint passed to \c set_endpoint()
	 * \param port The port passed to \c set_endpoint()
	 *
	 * \throw slirc::exceptions::already_connected if the connections state
	 *        is not state::disconnected
	 */
	inline void connect(const std::string &endpoint, unsigned port=0) {
		set_endpoint(endpoint, port);
		connect();
	}

	virtual void connect() override;
	virtual void disconnect() override;
	virtual state current_state() override;

protected:
	virtual void do_send_raw(const char *data, std::size_t length) override;
};

SLIRC_REGISTER_EVENT_ID_ENUM(connection::events);

}
}

#endif // SLIRC_MODULES_CONNECTION_HPP_INCLUDED
