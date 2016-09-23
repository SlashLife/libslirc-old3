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
namespace detail {
	struct module_base {
		module_base(::slirc::irc &irc_)
		: irc(irc_) {}
		virtual ~module_base()=default;

		::slirc::irc &irc;
	};
}

template<typename ModuleApi>
struct module: protected detail::module_base {
	friend struct ::slirc::irc;
	typedef ModuleApi module_api_type;

protected:
	module(::slirc::irc &irc_)
	: module_base(irc_) {}
};

}

#endif // SLIRC_MODULE_HPP_INCLUDED
