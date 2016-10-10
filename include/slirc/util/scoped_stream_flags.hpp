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

#ifndef SLIRC_UTIL_SCOPED_STREAM_FLAGS_HPP_INCLUDED
#define SLIRC_UTIL_SCOPED_STREAM_FLAGS_HPP_INCLUDED

#include "../detail/system.hpp"

#include <ios>
#include <string>

namespace slirc {
namespace util {

/** \brief Temporarily changes the format flags of a stream.
 *
 * On initializing an instance of this type, it will back up the current format
 * flags of a stream.
 *
 * On destruction the old flags will be restored.
 */
template<class CharT, class Traits = std::char_traits<CharT>
>struct scoped_stream_flags {
	/// The type of stream for which the format is backed up.
	typedef ::std::basic_ios<CharT, Traits> ios_type;

	/** \brief Backs up the format flags for a stream.
	 *
	 * \param stream The stream for which to back up and later restore the flags.
	 */
	scoped_stream_flags(ios_type &stream)
	: scoped_stream(stream)
	, original_flags(nullptr) {
		original_flags.copyfmt(scoped_stream);
	}

	/** \brief Restores the original format flags to the stream.
	 */
	~scoped_stream_flags() {
		restore();
	}

	/** \brief Restores the original format flags to the stream.
	 */
	void restore() {
		scoped_stream.copyfmt(original_flags);
	}
private:
	ios_type &scoped_stream;
	ios_type original_flags;
};

}
}

#endif // SLIRC_UTIL_SCOPED_STREAM_FLAGS_HPP_INCLUDED

