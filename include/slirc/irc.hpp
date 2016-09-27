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

#ifndef SLIRC_IRC_HPP_INCLUDED
#define SLIRC_IRC_HPP_INCLUDED

#include "detail/system.hpp"

#include <map>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <type_traits>

#include <boost/utility.hpp>

#include "component_container.hpp"
#include "event.hpp"
#include "module.hpp"
#include "apis/event_manager.hpp"

namespace slirc {

/** \brief An IRC context.
 *
 * This class models an IRC context, i.e. a single IRC connection.
 *
 * All modules associated with an IRC connection (including the module
 * representing the actual network connection itself) are loaded into this
 * class.
 *
 * Many entities in slirc are associated with a specific IRC context, e.g.
 * events and modules.
 */
class SLIRCAPI irc: public takes_components, private boost::noncopyable {
	irc(const irc &)=delete; // -Weffc++
	irc &operator=(const irc &)=delete; // -Weffc++

	typedef std::map<std::type_index, std::unique_ptr<module_base>> module_container_type;
	module_container_type modules_;
	apis::event_manager *event_manager_;

	template<typename Module>
	static constexpr bool is_valid_module_type() {
		return
			std::is_base_of<typename Module::module_base_api_type, Module>::value &&
			std::is_base_of<module<typename Module::module_base_api_type>, typename Module::module_base_api_type>::value &&
			std::is_base_of<module_base, module<typename Module::module_base_api_type>>::value;
	}

	template<typename Module>
	inline std::type_index module_index() { return typeid(typename Module::module_base_api_type); }

	// module api backend
	module_base *find_(std::type_index);
	void load_(std::type_index, std::unique_ptr<module_base>);
	bool unload_(std::type_index);

	template<typename Module>
	inline module_base *find_() { return find_(module_index<Module>()); }

public:
	/** \brief Constructs an irc context.
	 *
	 * \note After construction is finished, the IRC context will contain a
	 *       module implementing slirc::apis::event_manager.
	 */
	irc();

	/** \brief Destructs an irc context.
	 *
	 * \note During destruction, all modules are unloaded with the
	 *       active implementation of slirc::apis::event_manager being
	 *       unloaded last.
	 */
	~irc();



	// Module API

	/** \brief Loads a new module into the irc context.
	 *
	 * Constructs and loads a module of the specified type. The modules
	 * constructor will be called with \c *this as the first argument.
	 * If you pass any arguments to this function they will be appended to the
	 * constructor call.
	 *
	 * If the constructor throws an exception, the state of the irc context is
	 * not changed and the exception is handed through to the caller.
	 *
	 * If a module implementing the same \c module_base_api_type is loaded
	 * into this irc context already, it is not being replaced. No new module
	 * is constructed and an exception of type slirc::exceptions::module_conflict
	 * is thrown.
	 *
	 * \tparam Module The type of the module to load.
	 *
	 * \tparam Args... The types of the additional arguments to be passed to
	 *         the modules constructor.
	 *
	 * \param args Additional arguments to be passed to the modules constructor.
	 *
	 * \return A reference to the newly constructed module.
	 *
	 * \throw slirc::exceptions::module_conflict if another module of the
	 *     same \c module_base_api_type is loaded.
	 *
	 * \throw any exception thrown during construction of the module
	 */
	template<typename Module, typename... Args>
	Module &load(Args&& ...args) {
		static_assert(is_valid_module_type<Module>(), "Must be called with a valid module type!");
		if (find_<Module>()) throw exceptions::module_conflict();

		Module *realmod=nullptr;
		{ std::unique_ptr<module_base> mod(realmod = new Module(*this, std::forward<Args>(args)...));
			load_(module_index<Module>(), std::move(mod));
		}

		if (std::is_base_of<apis::event_manager, Module>::value) {
			event_manager_ = static_cast<apis::event_manager*>(static_cast<module_base*>(realmod));
		}

		SLIRC_ASSERT( realmod && "Module was not constructed (but did not throw)?!" );
		return *realmod;
	}

	/** \brief Unloads a module from the irc context.
	 *
	 * Unloads the module with the same \c module_base_api_type as the given
	 * module iff \c Module is either the exact type of the module currently
	 * loaded or a derived class and returns true\c .
	 *
	 * If no module with the same \c module_base_api_type is loaded, this
	 * function returns \c false. If one is loaded, but does not fulfill the
	 * mentioned criteria, an exception of type slirc::exception::module_conflict
	 * is thrown.
	 *
	 * Regardless of the return value, if this function does not throw, the
	 * slot for the given \c module_base_api_type will be free after this
	 * function returns.
	 *
	 * To ensure that a module occupying a certain slot is unloaded, you can
	 * call this function to unload the \c module_base_api_type directly, in
	 * which case no exception will be thrown:
	 * \code
	 *     slirc::irc irc;
	 *     irc.unload<some_module::module_base_api_type>();
	 * \endcode
	 *
	 * \tparam Module The type of the module to be unloaded.
	 *
	 * \return
	 *     - \c true if the module was unloaded,
	 *     - \c false if no module with the requested \c module_base_api_type
	 *          was loaded
	 */
	template<typename Module>
	bool unload() {
		static_assert(is_valid_module_type<Module>(), "Must be called with a valid module type!");

		auto modp = find_<Module>();
		if (!modp) return false;

		if (!dynamic_cast<Module*>(modp)) {
			throw exceptions::module_conflict();
		}

		if (std::is_base_of<apis::event_manager, Module>::value) {
			event_manager_ = nullptr;
		}

		return unload_(module_index<Module>());
	}

	/** \brief Finds a module within the irc context.
	 *
	 * If a module of the requested modules \c module_base_api_type is found
	 * and compatible (same or derived from) \c Module, a pointer to it is
	 * returned. Otherwise retuns a \c nullptr.
	 *
	 * \tparam Module The type of the module to find.
	 *
	 * \return
	 *     - \c a pointer to the loaded module,
	 *     - \c nullptr if no module with the same \c module_base_api_type is
	 *          loaded or the loaded module is not derived from \c Module
	 */
	template<typename Module>
	Module *find() {
		static_assert(is_valid_module_type<Module>(), "Must be called with a valid module type!");
		return dynamic_cast<Module*>(find_<Module>());
	}

	/** \brief Finds a module within the irc context.
	 *
	 * If a module of the requested modules \c module_base_api_type is found
	 * and compatible (same or derived from) \c Module, a pointer to it is
	 * returned. Otherwise retuns a \c nullptr.
	 *
	 * \tparam Module The type of the module to find.
	 *
	 * \return
	 *     - \c a pointer to the loaded module,
	 *     - \c nullptr if no module with the same \c module_base_api_type is
	 *          loaded or the loaded module is not derived from \c Module
	 */
	template<typename Module>
	const Module *find() const {
		return const_cast<irc&>(*this).find<Module>();
	}

	/** \brief Returns a module within the irc context.
	 *
	 * If a module of the requested modules \c module_base_api_type is found
	 * and compatible (same or derived from) \c Module, a reference to it is
	 * returned. Otherwise throws a \c std::range_error if no module was found
	 * or an slirc::exceptions::module_conflict if the module found is not
	 * derived from \c Module.
	 *
	 * \tparam Module The type of the module to return.
	 *
	 * \return A reference to the requested module.
	 */
	template<typename Module>
	Module &get() {
		static_assert(is_valid_module_type<Module>(), "Must be called with a valid module type!");

		module_base *rawmod = find_<Module>();
		if (!rawmod) throw std::range_error("Requested module not found.");

		Module *realmod = dynamic_cast<Module*>(rawmod);
		if (!realmod) throw exceptions::module_conflict();

		return *realmod;
	}

	/** \brief Returns a module within the irc context.
	 *
	 * If a module of the requested modules \c module_base_api_type is found
	 * and compatible (same or derived from) \c Module, a reference to it is
	 * returned. Otherwise throws a \c std::range_error if no module was found
	 * or an slirc::exceptions::module_conflict if the module found is not
	 * derived from \c Module.
	 *
	 * \tparam Module The type of the module to return.
	 *
	 * \return A reference to the requested module.
	 */
	template<typename Module>
	const Module &get() const {
		return const_cast<irc&>(*this).get<Module>();
	}



	// event api

	/** \brief Gets the event queue for this IRC context.
	 *
	 * Equivalent to <tt>get<apis::event_manager>()</tt>
	 *
	 * \return The event manager for this irc context.
	 */
	inline apis::event_manager &event_manager() {
		SLIRC_ASSERT( event_manager_ &&
			"IRC context should never be without a loaded event manager module!" );
		return *event_manager_;
	}

	/** \brief Gets the event queue for this IRC context.
	 *
	 * Equivalent to <tt>get<apis::event_manager>()</tt>
	 *
	 * \return The event manager for this irc context.
	 */
	inline const apis::event_manager &event_manager() const {
		SLIRC_ASSERT( event_manager_ &&
			"IRC context should never be without a loaded event manager module!" );
		return *event_manager_;
	}

	/** \brief Creates an event associated with this IRC context.
	 *
	 * \param id The original event type id identifying the new event.
	 *
	 * \return A pointer to the new event.
	 */
	inline event::pointer make_event(event::id_type id) {
		return event::make_event(*this, id);
	}
};

}

#endif // SLIRC_IRC_HPP_INCLUDED
