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

#ifndef SLIRC_UTIL_SCOPED_SWAP_HPP_INCLUDED
#define SLIRC_UTIL_SCOPED_SWAP_HPP_INCLUDED

#include "../detail/system.hpp"

#include <utility>

namespace slirc {
namespace util {

/** \brief Temporarily changes the value of a variable.
 *
 * On initializing an instance of this type, it will back up the current value
 * of a variable and replace it with a given new value.
 *
 * On destruction the old value will be restored.
 *
 * If possible, values will be moved.
 */
template<typename T>
struct scoped_swap {
	/** \brief Backs up the variable and sets it to a new value.
	 *
	 * \tparam U The type of the value to set the variable to.
	 *
	 * \param var The variable to swap.
	 * \param new_value A new value to set the variable to.
	 */
	template<typename U>
	scoped_swap(T &var, U new_value)
	: var_(var)
	, original_value_(std::move(var)) {
		var_ = std::move(new_value);
	}

	/// \brief Restores the original value to the variable.
	~scoped_swap() {
		var_ = std::move(original_value_);
	}

	/** \brief Check the original value.
	 *
	 * \return A reference to the original value.
	 */
	inline T &original_value() {
		return original_value;
	}

	/** \brief Check the original value.
	 *
	 * \return A reference to the original value.
	 */
	inline const T &original_value() const {
		return original_value;
	}
private:
	scoped_swap() = delete;
	scoped_swap(const scoped_swap &) = delete;
	scoped_swap &operator=(const scoped_swap &) = delete;

	T &var_;
	T original_value_;
};

}
}

#endif // SLIRC_UTIL_SCOPED_SWAP_HPP_INCLUDED
