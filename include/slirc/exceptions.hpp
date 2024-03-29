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

#ifndef SLIRC_EXCEPTIONS_HPP_INCLUDED
#define SLIRC_EXCEPTIONS_HPP_INCLUDED

#include "detail/system.hpp"

#include <stdexcept>

namespace slirc {
namespace exceptions {

/** \brief Is thrown when attempting to connect() an establishing or already
 *         established connection.
 *
 * \see slirc::apis::connection
 */
struct already_connected: std::logic_error {
	already_connected(): std::logic_error("The connection already is being established or has been established.") {}
};

/** \brief Signals the presence of a conflicting component on an operation on
 *    slirc::component_container.
 *
 * Thrown by slirc::component_container when the requested operation could not
 * be completed due to the presence of a conflicting component in the container.
 *
 * On insertion, a conflicting component is any component derived from the same
 * specialization of slirc::component.
 *
 * On retrieval (\c at, \c at_or_insert) or removal (\c remove) operations, a
 * conflicting component is one that is not identical to or derived from the
 * requested component type.
 *
 * To bypass failure by conflicts on retrieval or removal operations, you can
 * run the operation on the components base type instead.
 *
 * \see slirc::component::component_base_type
 */
struct component_conflict: std::logic_error {
	component_conflict(): std::logic_error("The component_container contains a conflicting component.") {}
};

/** \brief Is thrown if an invalid type id is used where a valid type id is required.
 */
struct invalid_event_id: std::logic_error {
	invalid_event_id(): std::logic_error("An invalid type id was used where a valid type id is required.") {}
};

/** \brief Signals the presence of a conflicting module on an operation on
 *    slirc::irc.
 *
 * Thrown by slirc::irc when the requested operation could not be completed due
 * to the presence of a conflicting module in the irc context.
 *
 * On \c load, a conflicting module is any module derived from the same
 * slirc::module::module_base_api_type.
 *
 * On retrieval (\c get) or removal (\c unload) operations, a conflicting module
 * is one that is not identical to or derived from the requested module type.
 *
 * To bypass failure by conflicts on retrieval or removal operations, you can
 * run the operation on the modules \c module_base_api_type instead.
 *
 * \see slirc::module::module_base_api_type
 */
struct module_conflict: std::logic_error {
	module_conflict(): std::logic_error("The irc context contains a conflicting module.") {}
};

}
}

#endif // SLIRC_EXCEPTIONS_HPP_INCLUDED

