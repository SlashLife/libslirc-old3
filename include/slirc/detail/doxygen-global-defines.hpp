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

#ifndef SLIRC_DOXYGEN
#	error "This file exists for documentation purposes only. Do not include it!"
#endif

#undef SLIRC_DEBUG
#define SLIRC_DEBUG
/** \def SLIRC_DEBUG
 *
 * \brief enables debugging in libslirc
 *
 * When defined, libslirc will do additional checks and internal asserts to aid
 * debugging.
 *
 * \note If the standard \a assert() is disabled, libslirc will still enable
 *       additional checks, but no assertions will be done.
 */

#undef SLIRCAPI
#define SLIRCAPI
/** \def SLIRCAPI
 *
 * \brief marks the exported API
 */

#undef SLIRC_BUILD_NO_SSL
#define SLIRC_BUILD_NO_SSL
/** \def SLIRC_BUILD_NO_SSL
 *
 * \brief Build libslirc without SSL.
 */

#undef SLIRC_NO_INCLUDE_API_DEFAULT_IMPLEMENTATION
#define SLIRC_NO_INCLUDE_API_DEFAULT_IMPLEMENTATION
/** \def SLIRC_NO_INCLUDE_API_DEFAULT_IMPLEMENTATION
 *
 * \brief Disables automatic includesion of default implementations for module APIs
 *
 * When defined, including any member of slirc::apis will not automatically
 * include its default implementation.
 */
