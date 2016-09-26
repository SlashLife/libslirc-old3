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

#ifndef SLIRC_MODULE_HPP_INCLUDED
#define SLIRC_MODULE_HPP_INCLUDED

#include <cassert>

namespace slirc {

/** \brief Defines the base interface for modules.
 */
struct SLIRCAPI module_base {
	/** \brief A reference to the IRC context this module is loaded into.
	 */
	::slirc::irc &irc;
	virtual ~module_base()=default;

protected:
	/** \brief Constructs the module base.
	 */
	module_base(::slirc::irc &irc_)
	: irc(irc_) {}
};

/** \brief Defines the base for specific modules.
 *
 * \note Only one module with the same \c module_base_api_type can be loaded
 *       into the same IRC context at any time.
 */
template<typename ModuleApi>
struct module: public module_base {
	friend struct ::slirc::irc;

	/// \brief The base
	typedef ModuleApi module_base_api_type;

protected:
	using module_base::module_base;
};

}

#endif // SLIRC_MODULE_HPP_INCLUDED
