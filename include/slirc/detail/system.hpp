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

#ifndef SLIRC_DETAIL_SYSTEM_HPP_INCLUDED
#define SLIRC_DETAIL_SYSTEM_HPP_INCLUDED

#include <cassert>

// declare globally used friend for deep introspection tests
namespace slirc { namespace test { class test_overrides; }}

#undef SLIRCAPI
#ifdef SLIRC_EXPORTS
#	define SLIRCAPI __declspec(dllexport)
#else
#	define SLIRCAPI __declspec(dllimport)
#endif

#define SLIRC_COMMA ,

// Rationale: Keeping the return type in Doxygen clean of std::enable_if.
// Instead the conditions on which functions are enabled should be documented in
// a Doxygen \note. This macro is overridden by the Doxygen configuration.
#define SLIRC_ENABLE_IF(condition, return_type) std::enable_if_t<condition, return_type>

#ifndef SLIRC_ASSERT
// use own version to allow test cases to check whether assertions are hit
#	ifndef SLIRC_DEBUG
#		define SLIRC_ASSERT(cond) (void(0))
#	else
#		define SLIRC_ASSERT(cond) assert(cond)
#	endif
#endif // SLIRC_ASSERT

#endif // SLIRC_DETAIL_SYSTEM_HPP_INCLUDED

