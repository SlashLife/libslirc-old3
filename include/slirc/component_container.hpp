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

#ifndef SLIRC_COMPONENT_CONTAINER_HPP_INCLUDED
#define SLIRC_COMPONENT_CONTAINER_HPP_INCLUDED

#include "detail/system.hpp"

#include <cassert>

#include <memory>
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "component.hpp"
#include "exceptions.hpp"
#include "util/noncopyable.hpp"

namespace slirc {

namespace detail {
	template<typename T>
	constexpr bool is_valid_component() {
		return
			std::is_base_of< // 1 (see test)
				::slirc::detail::component_base,
				::slirc::component<typename T::component_base_type>
			>::value &&
			std::is_base_of< // 2 (see test)
				::slirc::component<typename T::component_base_type>,
				typename T::component_base_type
			>::value &&
			std::is_base_of< // 3 (see test)
				typename T::component_base_type,
				T
			>::value &&
			!std::is_same< // 4 (see test)
				::slirc::component<typename T::component_base_type>,
				T
			>::value;
	}
}



/** \brief Stores components
 *
 * This container can store up to one of any base component type.
 *
 * All components must be derived from slirc::component.
 *
 * Two components that derive from the same specialization of slirc::component
 * are considered conflicting and can not be stored in the same container at
 * the same time.
 */

struct component_container: private util::noncopyable {
	friend class ::slirc::test::test_overrides;

private:
	typedef std::unordered_map<std::type_index, std::unique_ptr<detail::component_base>> contents_type;
	contents_type contents{};

	template<typename Component>
	contents_type::iterator find_() {
		return contents.find(typeid(typename Component::component_base_type));
	}

	template<typename Component>
	contents_type::const_iterator find_() const {
		return contents.find(typeid(typename Component::component_base_type));
	}

	template<typename Component>
	Component &insert_(Component &&value) {
		return static_cast<Component&>(*(
			contents[typeid(typename Component::component_base_type)] =
				contents_type::mapped_type(
					new Component(std::forward<Component&&>(value))
				)
		));
	}

public:
	/** \brief Inserts a new component.
	 *
	 * \tparam Component
	 *     The type of the component to be inserted.
	 *
	 * \param value The component to be inserted.
	 *
	 * \return A reference to the inserted component.
	 *
	 * \throw slirc::exceptions::component_conflict
	 *     if the container already contains a conflicting component.
	 *
	 * \note When being inserted, the component will be copied (or moved).
	 */
	template<typename Component>
	Component &insert(Component &&value = Component{}) {
		static_assert(detail::is_valid_component<Component>(), "Supplied type for Component is not valid as a component.");

		if (find_<Component>() != contents.end()) {
			throw exceptions::component_conflict();
		}

		return insert_<Component>(std::forward<Component&&>(value));
	}

	/** \brief Fetches an existing component or inserts a new component.
	 *
	 * \tparam Component
	 *     The type of the component to be requested or inserted.
	 * \tparam Unused
	 *     Unused template parameter (implementation detail). If supplied, it
	 *     must be the same as Component.
	 *
	 * \param value The component to be inserted.
	 *
	 * If the container contains a component of the same base component type,
	 * it will be either be returned if it is compatible with the requested
	 * type, or an exception of type slirc::exceptions::component_conflict will be thrown.
	 *
	 * Otherwise the passed component will be inserted and a reference to the
	 * inserted component is returned.
	 *
	 * This variant allows a component of a more derived type to be inserted
	 * than the type that is requested for returning.
	 *
	 * \return A reference to the inserted component.
	 *
	 * \throw slirc::exceptions::component_conflict
	 *     if the container already contains a conflicting component.
	 *
	 * \note When being inserted, the component will be copied (or moved).
	 */
	template<typename Component, typename Unused=Component>
	SLIRC_ENABLE_IF(std::is_same<Component SLIRC_COMMA Unused>::value
	, Component) &at_or_insert(Component &&value = Component{}) {
		static_assert(detail::is_valid_component<Component>(), "Supplied type for Component is not valid as a component.");

		try {
			return at<Component>();
		}
		catch(std::out_of_range&) {
			return insert_<Component>(std::forward<Component&&>(value));
		}
	}

	/** \brief Fetches an existing component or inserts a new component.
	 *
	 * \tparam RequestedComponent
	 *     The type of the component to be requested.
	 * \tparam NewComponent
	 *     The type of the component that is inserted if no component of the
	 *     requested type is available.
	 *
	 * \param value The component to be inserted.
	 *
	 * If the container contains a component of the same base component type,
	 * it will be either be returned if it is compatible with the requested
	 * type, or an exception of type slirc::exceptions::component_conflict will
	 * be thrown.
	 *
	 * Otherwise the passed component will be inserted and a reference to the
	 * inserted component is returned.
	 *
	 * This variant allows a component of a more derived type to be inserted
	 * than the type that is requested for returning.
	 *
	 * \return A reference to the inserted component.
	 *
	 * \throw slirc::exceptions::component_conflict
	 *     if the container already contains a conflicting component.
	 *
	 * \note When being inserted, the component will be copied (or moved).
	 */
	template<typename Component, typename NewComponent>
	SLIRC_ENABLE_IF(!std::is_same<Component SLIRC_COMMA NewComponent>::value
	, Component) &at_or_insert(NewComponent &&value = NewComponent{}) {
		static_assert(detail::is_valid_component<Component>(), "Supplied type for Component is not valid as a component.");
		static_assert(detail::is_valid_component<NewComponent>(), "Supplied type for NewComponent is not valid as a component.");
		static_assert(std::is_base_of<Component, NewComponent>::value, "NewComponent must be more derived than Component.");

		try {
			return at<Component>();
		}
		catch(std::out_of_range&) {
			return insert_<NewComponent>(std::forward<NewComponent&&>(value));
		}
	}

	/** \brief Fetches a component.
	 *
	 * \tparam Component
	 *     The type of the component to be fetched.
	 *
	 * If the container contains a component of the same base component type,
	 * it will be either be returned if it is compatible with the requested
	 * type, or an exception of type slirc::exceptions::component_conflict will
	 * be thrown.
	 *
	 * If no component of the same base type is found, an exception of type
	 * std::out_of_range is thrown.
	 *
	 * \return A reference to the requested component.
	 *
	 * \throw std::out_of_range
	 *     if the container contains no component of the same base type.
	 * \throw slirc::exceptions::component_conflict
	 *     if the container contains a conflicting component.
	 */
	template<typename Component>
	Component &at() {
		static_assert(detail::is_valid_component<Component>(), "Supplied type for Component is not valid as a component.");

		auto it = find_<Component>();

		if (it == contents.end()) {
			throw std::out_of_range("No such component");
		}
		try {
			return dynamic_cast<Component&>(
				static_cast<component<typename Component::component_base_type>&>(*it->second));
		}
		catch(std::bad_cast&) {
			throw exceptions::component_conflict();
		}
	}

	/** \brief Fetches a component.
	 *
	 * \tparam Component
	 *     The type of the component to be fetched.
	 *
	 * If the container contains a component of the same base component type,
	 * it will be either be returned if it is compatible with the requested
	 * type, or an exception of type slirc::exceptions::component_conflict will
	 * be thrown.
	 *
	 * If no component of the same base type is found, an exception of type
	 * std::out_of_range is thrown.
	 *
	 * \return A reference to the requested component.
	 *
	 * \throw std::out_of_range
	 *     if the container contains no component of the same base type.
	 * \throw slirc::exceptions::component_conflict
	 *     if the container contains a conflicting component.
	 */
	template<typename Component>
	const Component &at() const {
		return const_cast<component_container&>(*this).at<Component>();
	}

	/** \brief Finds a component.
	 *
	 * \tparam Component
	 *     The type of the component to be fetched.
	 *
	 * \return
	 *     A pointer to the requested component, if a compatible component is
	 *     stored. nullptr otherwise.
	 */
	template<typename Component>
	Component *find() {
		static_assert(detail::is_valid_component<Component>(), "Supplied type for Component is not valid as a component.");

		auto it = find_<Component>();
		if (it == contents.end()) {
			return nullptr;
		}
		return dynamic_cast<Component*>(
			static_cast<component<typename Component::component_base_type>*>(it->second.get()));
	}

	/** \brief Finds a component.
	 *
	 * \tparam Component
	 *     The type of the component to be fetched.
	 *
	 * \return
	 *     A pointer to the requested component, if a compatible component is
	 *     stored. nullptr otherwise.
	 */
	template<typename Component>
	const Component *find() const {
		return const_cast<component_container&>(*this).find<Component>();
	}

	/** \brief Checks whether the container contains a specific component.
	 *
	 * \tparam Component
	 *     The type of the component to be checked for.
	 *
	 * \return
	 *     true if the container contains a compatible component, false otherwise.
	 */
	template<typename Component>
	bool has() const {
		return find<Component>();
	}

	/** \brief Removes a component.
	 *
	 * \tparam Component
	 *     The type of the component to be removed.
	 *
	 * \return
	 *     true if a component has been removed,
	 *     false if no matching component was found.
	 *
	 * \throw slirc::exceptions::component_conflict
	 *     if the container contains a conflicting component.
	 *
	 * \note
	 *     If the container contains a mismatching component (i.e.: if at()
	 *     would throw slirc::exceptions::component_conflict), it will not be
	 *     removed and subsequent calls to insert<component>() will fail.
	 *     To remove any component occupying the slot, call
	 *     remove<Component::component_base_type>() instead.
	 */
	template<typename Component>
	bool remove() {
		static_assert(detail::is_valid_component<Component>(), "Supplied type for Component is not valid as a component.");

		auto it = find_<Component>();
		if (it == contents.end()) {
			return false;
		}
		if (dynamic_cast<Component*>(static_cast<component<
			typename Component::component_base_type>*>(it->second.get()))) {
			contents.erase(it);
			return true;
		}
		throw exceptions::component_conflict();
	}
};

/** \brief Enables derived classes to hold components.
 */
struct takes_components {
	/// \brief Stores components
	component_container components;

protected:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
	/// \brief Constructs the component holder.
	takes_components() = default;
	/// \brief Destructs the component holder.
	~takes_components() = default;
#pragma GCC diagnostic pop
};

}

#endif // SLIRC_COMPONENT_CONTAINER_HPP_INCLUDED
