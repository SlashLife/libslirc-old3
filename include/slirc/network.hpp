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

#ifndef SLIRC_NETWORK_HPP_INCLUDED
#define SLIRC_NETWORK_HPP_INCLUDED

#include "detail/system.hpp"

namespace boost { namespace asio {
	struct io_service;
}}

namespace slirc {
namespace network {

/** \brief Get ASIO service.
 *
 * This will return the service set using \c set_service() or the internal
 * service if no service was set.
 *
 * If the internal service is being used, but not running, then calling this
 * function will also start the internal service in a separate thread. The
 * service will be kept running until slirc is unloaded. During unloading,
 * slirc will block until the service stopped running and its thread was joined.
 *
 * \return An ASIO service.
 */
SLIRCAPI boost::asio::io_service &service();

/** \brief Set ASIO service.
 *
 * Sets an external ASIO service to be used.
 *
 * \note If the internal ASIO service has already been requested, setting an
 *       external ASIO service will not stop the internal service or its thread,
 *       nor will any work from the internal service be automatically shifted
 *       to the external one.
 */
SLIRCAPI void service(boost::asio::io_service &external_service);

/** \brief Checks whether slirc uses its internal ASIO service.
 *
 * \return
 *     - \c true if the internal service is being used,
 *     - \c false if an external service was set.
 */
SLIRCAPI bool uses_internal_service();

/** \brief Checks whether libslirc was built with SSL support.
 *
 * Checks whether libslirc was built with SSL support.
 *
 * If libslirc was built without SSL support, slirc::modules::connection will
 * not support the <tt>ssl://</tt> and <tt>ircs://</tt> protocols.
 *
 * \return
 *     - \c true if libslirc has SSL support,
 *     - \c false if it does not.
 */
SLIRCAPI bool has_ssl_support();

}
}

#endif // SLIRC_NETWORK_HPP_INCLUDED
