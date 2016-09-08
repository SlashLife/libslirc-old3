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

#ifndef SLIRC_COMPONENT_HPP_INCLUDED
#define SLIRC_COMPONENT_HPP_INCLUDED

#include "detail/system.hpp"

namespace slirc {

struct component_container;

namespace detail {
	struct component_base {
		virtual ~component_base() = default;
	};
}

/** \brief
 *     The base class for components to be used with
 *     slirc::component_container.
 *
 * \tparam ComponentBaseType
 *     The type that has been derived directly from this class.
 *
 * To create a new component, derive from this class using your component base
 * type as the template parameter (CRTP), i.e. if you want to create a new
 * component \c my_component, derive from \c component<my_component>.
 *
 * When extending a component, you can just derive from the base component, so
 * \c my_derived_component should inherit from \c my_component, not directly
 * from this class.
 */
template<typename ComponentBaseType>
struct component: private detail::component_base {
	friend struct ::slirc::component_container;

	/**
	 * \brief The base type for this component
	 *
	 * \note Components with the same base type are mutually exclusive in an
	 *     slirc::component_container
	 */
	typedef ComponentBaseType component_base_type;
};

}

#endif // SLIRC_COMPONENT_HPP_INCLUDED
