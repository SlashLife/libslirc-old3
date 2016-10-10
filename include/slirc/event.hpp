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

#ifndef SLIRC_EVENT_HPP_INCLUDED
#define SLIRC_EVENT_HPP_INCLUDED

#include "detail/system.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <ostream>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <vector>

#if __has_include(<optional>)
#	include <optional>
#elif __has_include(<experimental/optional>)
#	include <experimental/optional>
#else
#	error Neither std::optional nor std::experimental::optional are supported
#endif

#include "component_container.hpp"
#include "exceptions.hpp"
#include "util/noncopyable.hpp"
#include "util/scoped_stream_flags.hpp"

/** \def SLIRC_REGISTER_EVENT_ID_ENUM(idtype)
 *
 * \brief Enables usage of an enum type as event ids.
 *
 * \param idtype The (possibly qualified) name of the
 *               enum type to be used as event ids.
 *
 * \code
 *     namespace my_namespace {
 *         enum some_events: slirc::event::underlying_id_type { event1 };
 *         enum more_events: slirc::event::underlying_id_type { event2 };
 *
 *         SLIRC_REGISTER_EVENT_ID_ENUM( some_events );
 *     }
 *     // also possible:
 *     SLIRC_REGISTER_EVENT_ID_ENUM( my_namespace::more_events );
 * \endcode
 */

#undef SLIRC_REGISTER_EVENT_ID_ENUM
#define SLIRC_REGISTER_EVENT_ID_ENUM(idtype) \
	static_assert(std::is_same< \
		std::underlying_type<idtype>::type, ::slirc::event::underlying_id_type \
	>::value, \
		#idtype " is of wrong underlying type to be used as an event id type"); \
	constexpr std::true_type slirc_impldetail_enable_as_event_id_type(idtype)

namespace slirc {

class irc;

// only declared, never defined!
template<typename NotAnEventId>
constexpr std::false_type slirc_impldetail_enable_as_event_id_type(NotAnEventId);

/** \brief An IRC event.
 *
 * An event can describe anything that is happening in an IRC context.
 *
 * Events consist of three main components:
 *   - They are associated with a specific IRC context.
 *   - They have an event id which represents the type of event. While they
 *     start out with a specific event id, the original_id, they can (and in
 *     most cases will) go through several different event types during their
 *     lifetime, their current_id.
 *   - Additional components to describe the event.
 *
 * While the associated IRC context and original event id are fixed, the current
 * event id and the components can be changed. Arbitrary components and event
 * ids can be created added; for the specific requirements for components, check
 * the documentation of the \ref component class; to add new event ids, check
 * the documentation of the event::id_type class.
 *
 * The life cycle of an event begins with its creation: To create a new event,
 * request it from your IRC context:
 * \code
 *   #include <slirc/event.hpp>
 *   #include <slirc/irc.hpp>
 *
 *   enum my_events {
 *   	my_event_id
 *   };
 *   SLIRC_REGISTER_EVENT_ID_ENUM(my_events);
 *
 *   slirc::irc irc;
 *   slirc::event::pointer e = irc.make_event(my_event_id);
 * \endcode
 */
class SLIRCAPI event: public takes_components, public std::enable_shared_from_this<event>, private util::noncopyable {
public:
	typedef std::shared_ptr<event> pointer; ///< A shared pointer to an event.
	typedef std::weak_ptr<event> weak_pointer; ///< A weak pointer to an event.

	/** The underlying type that enums must have to be eligible as event ids.
	 */
	typedef unsigned underlying_id_type;

	/// \brief Specifies which end of the queue to add the event type to
	enum queuing_position {
		at_back, ///< add type to end of queue (handle last)
		at_front ///< add type to front of queue (handle next)
	};

	/// \brief Specifies what to do with the new type id if the type is queued already
	enum queuing_strategy {
		discard,  ///< discard new type id, keep old
		replace,  ///< replace old type id, keep new
		duplicate ///< insert duplicate, keep both
	};

	/// \brief Specifies the result of a queuing operation
	enum queuing_result {
		discarded, ///< The requested event id was not inserted because of an already queued type id
		queued,    ///< The requested event id has been added
		replaced,  ///< The requested event id has been added, but any equivalent id has been removed
		invalid    ///< The requested event id was not inserted because it is invalid;
		           ///< this is only used as a status for range based \ref queue_as overload callbacks
	};



	/** \brief Checks whether a value is a valid event id type.
	 * \relates slirc::event::id_type
	 *
	 * \tparam IdType The type to be checked.
	 *
	 * \return
	 *     - <tt>true</tt> if the passed type is a valid event id type,
	 *     - <tt>false</tt> otherwise
	 *
	 * \note See the description of slirc::event::id_type for details about
	 *     valid event id types.
	 */
	template<typename T>
	static inline constexpr bool is_valid_id_type() {
		return
			std::is_same<typename std::underlying_type<T>::type, event::underlying_id_type>::value &&
			decltype(slirc_impldetail_enable_as_event_id_type(std::declval<T>()))::value;
	}

	/** \brief Checks whether a value is a valid event id.
	 * \relates slirc::event::id_type
	 *
	 * \tparam IdType The type of the id to be checked.
	 *
	 * \param id The id to be checked.
	 *
	 * \return
	 *     - <tt>true</tt> if the passed id is a valid event id,
	 *     - <tt>false</tt> otherwise
	 *
	 * \note See the description of slirc::event::id_type for details about
	 *     valid event id types.
	 *
	 * \note This function will only check the type of the id; it will not
	 *     check whether it is actually a valid value for its enum type.
	 */
	template<typename IdType>
	static inline constexpr bool is_valid_id(IdType id) {
		((void)id); // unused parameter
		return is_valid_id_type<IdType>();
	}

	/** \brief Represents an event id.
	 *
	 * Event ids represent different types of events that modules can install
	 * listeners for.
	 *
	 * Event ids can be created from specific enums. Enums eligible to be used
	 * as event ids must fulfill two criteria:
	 *   - their underlying type must be event::underlying_id_type and
	 *   - they must specifically be registered as an event id type using the
	 *     SLIRC_REGISTER_EVENT_ID_ENUM() macro:
	 *
	 * \include example.event.id_type.enum.cpp
	 */
	struct id_type {
		friend struct ::slirc::test::test_overrides;
		friend struct std::less<id_type>;
		friend struct std::hash<id_type>;

		/** \brief Callback type for matching event ids.
		 */
		typedef std::function<bool(const id_type &)> matcher;

		/** \brief Constructs an invalid event id.
		 *
		 * \note Using an invalid event id in a context that requires a valid
		 *       event id may result in an exception of type
		 *       slirc::exceptions::invalid_event_id being thrown.
		 */
		id_type()
		: index()
		, id(0) {}

		/** \brief Creates a copy of an event id.
		 */
		id_type(const id_type &other)
		: index(other.index)
		, id(other.id) {}

		/** \brief Constructs an event id of a specific type.
		 *
		 * \tparam IdType The type of an enum registed for use as event ids.
		 *
		 * \param id The event id to be used.
		 *
		 * \note \a IdType is type checked. This constructor is only eligible
		 *       for conversion of enum types. However, to make finding bugs
		 *       easier, it will also accept enum types not registered as event
		 *       id types and fail compilation with a descriptive error message.
		 */
		template<typename IdType>
#ifdef SLIRC_DOXYGEN
		// don't list the enum type check as part of the API
		id_type(IdType id)
#else
		id_type(IdType id, typename std::enable_if<std::is_enum<IdType>::value, int>::type=0)
#endif
		: index(typeid(IdType))
		, id(static_cast<underlying_id_type>(id)) {
			static_assert(is_valid_id_type<IdType>(),
				"Passed value is of an invalid id type. \n"
				"Valid id types must \n"
				"  - be enums of underlying type slirc::event::underlying_id and \n"
				"  - registered as event id type using SLIRC_REGISTER_EVENT_ID_ENUM(enum_type_name)");
		}

		/** \brief Compares two event ids for equality.
		 *
		 * Checks whether two event ids are the same. To be considered the same,
		 * they must not only have the same numeric representation, but also
		 * originate from the same enum type.
		 *
		 * \param lhs the first event id
		 * \param rhs the second event id
		 *
		 * \return
		 *     - <tt>true</tt> if both ids are the same,
		 *     - <tt>false</tt> otherwise
		 */
		friend inline bool operator==(const id_type &lhs, const id_type &rhs) {
			return lhs.id==rhs.id && lhs.index==rhs.index;
		}

		/** \brief Compares two event ids for inequality.
		 *
		 * Checks whether two event ids differ. To be considered different, they
		 * must either have a different numeric representation, or originate
		 * from different enum types.
		 *
		 * \param lhs the first event id
		 * \param rhs the second event id
		 *
		 * \return
		 *     - <tt>true</tt> if both ids are the same,
		 *     - <tt>false</tt> otherwise
		 */
		friend inline bool operator!=(const id_type &lhs, const id_type &rhs) {
			return !(lhs==rhs);
		}

		/** \brief Checks whether the event id is valid.
		 *
		 * \return
		 *     - <tt>true</tt> if the id is valid (not equal to the default id),
		 *     - <tt>false</tt> otherwise
		 */
		explicit inline operator bool() const {
			return *this != id_type();
		}

		/** \brief Checks whether the event id is invalid.
		 *
		 * \return
		 *     - <tt>true</tt> if the id is invalid (equal to the default id),
		 *     - <tt>false</tt> otherwise
		 */
		inline bool operator!() const {
			return *this == id_type();
		}

		/** \brief Checks whether the event id originates from a certain type.
		 *
		 * \tparam IdType The enum type to check against.
		 *
		 * \return
		 *     - <tt>true</tt> if the id originates from IdType,
		 *     - <tt>false</tt> otherwise
		 */
		template<typename IdType>
		inline bool is_of_type() const {
			static_assert(is_valid_id_type<IdType>(),
				"Passed value is of an invalid id type. \n"
				"Valid id types must \n"
				"  - be enums of underlying type slirc::event::underlying_id and \n"
				"  - registered as event id type using SLIRC_REGISTER_EVENT_ID_ENUM(enum_type_name)");
			return index && *index == typeid(IdType);
		}

		/** \brief Checks whether the event id originates from a certain type.
		 *
		 * \tparam IdType The enum type to check against.
		 *
		 * \param id Some id from the enum type to check against
		 *
		 * \return
		 *     - <tt>true</tt> if the id originates from the same enum type as id,
		 *     - <tt>false</tt> otherwise
		 */
		template<typename IdType>
		inline bool is_of_type(IdType id) const {
			static_assert(is_valid_id_type<IdType>(),
				"Passed value is of an invalid id type. \n"
				"Valid id types must \n"
				"  - be enums of underlying type slirc::event::underlying_id and \n"
				"  - registered as event id type using SLIRC_REGISTER_EVENT_ID_ENUM(enum_type_name)");
			((void)id); // unused parameter
			return is_of_type<IdType>();
		}

		/** \brief Gets the enum value being used as the event id.
		 *
		 * \tparam IdType The enum type that is currently stored.
		 *
		 * \throw std::bad_cast if the currently stored event id is not of type IdType
		 *
		 * \return The currently stored enum value.
		 */
		template<typename IdType>
		inline IdType get() {
			if (!is_of_type<IdType>()) {
				throw std::bad_cast();
			}
			return static_cast<IdType>(id);
		}

		/** \brief Prints a string representation to an std::ostream for debugging.
		 *
		 * \param os The ostream to print to.
		 *
		 * \note The string representation uses std::type_info::name() and is
		 *       implementation dependent accordingly. Do not use this for any
		 *       purpose other than to get a human readable representation.
		 */
		void print_debug(std::ostream &os) const {
			util::scoped_stream_flags<char> ssf(os);
			os.copyfmt(std::basic_ios<char>(nullptr));

			if (!*this) {
				os << "<invalid>";
			}
			else {
				os << "<event: " << index->name() << ", " << id << '>';
			}
		}

	private:
#if __has_include(<optional>)
		std::optional
#elif __has_include(<experimental/optional>)
		std::experimental::optional
#endif
			<std::type_index> index;
		underlying_id_type id;
	};

	friend class ::slirc::irc;
	friend class ::slirc::test::test_overrides;

	::slirc::irc &irc; ///< The IRC context this event is associated with.

	const id_type original_id; ///< The original event id this event was created as.
	const id_type &current_id; ///< The event id this event is currently being handled as.

private:
	event()=delete;
	event(const event &)=delete;

	struct constructor_tag {};
	static inline pointer make_event(slirc::irc &irc_, id_type original_id_) {
		return std::make_shared<event>(constructor_tag(), irc_, original_id_);
	}

	id_type current_id_mutable;

	typedef std::vector<id_type> id_queue_type;
	id_queue_type queued_ids; // elements [0..next_id_index-1] are unused
	id_queue_type::size_type next_id_index;

public:
#ifndef SLIRC_DOXYGEN
	/// Not documented. Internal use only.
	event(constructor_tag, slirc::irc &irc_, id_type original_id_);
#endif // SLIRC_DOXYGEN

	/** \brief Kicks off handling of the event.
	 *
	 * Instructs the event queue module of the IRC context associated with
	 * this event to invoke all registered event handlers for all event ids
	 * queued up (currently or during the process of handling the event.)
	 *
	 * When this function returns, the event id queue will be empty.
	 */
	void handle();

	/** \brief Handles event as specific type id.
	 *
	 * Instructs the event queue module of the IRC context associated with
	 * this event to invoke all registered event handlers for the given
	 * event id.
	 *
	 * For the duration of the event handling, current_id will reflect the
	 * id passed.
	 *
	 * \param id The event id to handle this event as.
	 *
	 * \throw exceptions::invalid_event_id if the event id was invalid
	 */
	void handle_as(id_type id);

	/** \brief Queues event as a different id.
	 *
	 * Queues this event as a different id. If the given id is queued already,
	 * the result depends on the queuing strategy used.
	 *
	 * \param id The event id to queue this event as.
	 * \param strategy What to do in case of an existing duplicate of the
	 *     given event it:
	 *     - \c discard to not insert the event id if an equivalent event id
	 *            is queued already; discarded will be returned in this
	 *            case, queued otherwise,
	 *     - \c replace to remove any equivalent event id that is queued
	 *            already; replaced will be returned if any event id was
	 *            removed from the queue, queued otherwise
	 *     - \c duplicate to ignore existing equivalent event ids; queuing
	 *            an event with this strategy will return \c accepted.
	 * \param position Where in the queue to add the new event id:
	 *     - \c at_back to queue this event last in the queue,
	 *     - \c at_front to queue this event next in the queue.
	 *
	 * \return
	 *     - \c discarded if discard was chosen as the strategy and
	 *            a matching event id is queued already,
	 *     - \c accepted if the id was queued successfully,
	 *     - \c replaced if replace was chosen as the strategy and
	 *            a matching event id is queued already
	 *
	 * \throw exceptions::invalid_event_id if the event id was invalid
	 */
	queuing_result queue_as(id_type id, queuing_strategy strategy=discard, queuing_position position=at_back);

	/** \brief Queues event as a different id.
	 *
	 * Queues this event as a different id. If the given id is queued already,
	 * this request is discarded.
	 *
	 * \param id The event id to queue this event as.
	 * \param position Where in the queue to add the new event id:
	 *     - \c at_back to queue this event last in the queue,
	 *     - \c at_front to queue this event next in the queue.
	 *
	 * \return
	 *     - \c discarded if discard was chosen as the strategy and
	 *            a matching event id is queued already,
	 *     - \c accepted if the id was queued successfully,
	 *     - \c replaced if replace was chosen as the strategy and
	 *            a matching event id is queued already
	 *
	 * \throw exceptions::invalid_event_id if the event id was invalid
	 */
	inline queuing_result queue_as(id_type id, queuing_position position) {
		return queue_as(id, discard, position);
	}

	/** \brief Queues event as different ids.
	 *
	 * Queues the event as the ids specified by the interval \c begin..end.
	 *
	 * Equivalent to repeated calls to queue_as(id_type, ...), with two exceptions:
	 *   - when inserting \c at_front, the order of event ids will be preserved
	 *   - the queuing strategy will only be applied to the previously queued
	 *     event ids, so when queuing a range containing duplicates with the
	 *     \c replace strategy, duplicates in the newly added range will be
	 *     preserved (while duplicates previously queued will still be removed),
	 *     whereas with the \c discard strategy, duplicates in the new range
	 *     will either all be removed or all be preserved, depending on whether
	 *     or not the previous queue contained an equivalent event id.
	 *
	 * Instead of returning the result equivalent to queue_as(...),
	 * \c result_callback will be called with the current iterator and the
	 * result for each element in order.
	 *
	 * \tparam Iterator The type of the iterators representing the range.
	 *
	 * \param begin The begin of the range of event ids to be added
	 * \param end One past the end of the range of event ids to be added
	 * \param strategy What to do in case of an existing duplicate of the
	 *     given event it:
	 *     - \c discard to not insert the event id if an equivalent event id
	 *            is queued already; discarded will be returned in this
	 *            case, queued otherwise,
	 *     - \c replace to remove any equivalent event id that is queued
	 *            already; replaced will be returned if any event id was
	 *            removed from the queue, queued otherwise
	 *     - \c duplicate to ignore existing equivalent event ids; queuing
	 *            an event with this strategy will always return queued.
	 * \param position Where in the queue to add the new event id:
	 *     - \c at_back to queue this event last in the queue,
	 *     - \c at_front to queue this event next in the queue.
	 * \param result_callback A function that takes the iterator to an event id
	 *     as well as the result from attempting to queue this id.
	 */
	template<typename Iterator, typename ResultCallback = std::function<void(Iterator, queuing_result)>>
	inline void queue_as(
		Iterator begin, Iterator end,
		queuing_strategy strategy=discard,
		queuing_position position=at_back,
		ResultCallback result_callback
			= [](Iterator, queuing_result){}
	) {
		static_assert(std::is_base_of<
			std::input_iterator_tag,
			typename std::iterator_traits<Iterator>::iterator_category
		>::value, "Given iterator type must satisfy at least the input_iterator requirements.");

		// can only do this check if reading from the iterator will not destroy the input
		if (std::is_base_of<
			std::forward_iterator_tag,
			typename std::iterator_traits<Iterator>::iterator_category
		>::value) {
			// could happen for random access iterators - otherwise the assert will
			// loop indefinitely - but so would the function itself.
			SLIRC_ASSERT( 0 <= std::distance(begin, end) && "begin must not be after end" );
		}

		id_queue_type add_ids;
		while(begin != end) {
			auto result = prepare_append_queue(add_ids, *begin, strategy);
			result_callback(begin, result);
			++begin;
		}

		append_to_queue_unchecked(add_ids, position);
	}

	/** \brief Queues event as different ids.
	 *
	 * Queues the event as the ids specified by the interval \c begin..end.
	 *
	 * Equivalent to <tt>queue_as(\a begin, \a end, discard, \a position, \a result_callback)</tt>.
	 *
	 * \tparam Iterator The type of the iterators representing the range.
	 *
	 * \param begin The begin of the range of event ids to be added
	 * \param end One past the end of the range of event ids to be added
	 * \param position Where in the queue to add the new event id:
	 *     - \c at_back to queue this event last in the queue,
	 *     - \c at_front to queue this event next in the queue.
	 * \param result_callback A function that takes the iterator to an event id
	 *     as well as the result from attempting to queue this id.
	 */
	template<typename Iterator, typename ResultCallback = std::function<void(Iterator, queuing_result)>>
	inline void queue_as(
		Iterator begin, Iterator end,
		queuing_position position,
		ResultCallback result_callback
			= [](Iterator, queuing_result){}
	) {
		queue_as(begin, end, discard, position, result_callback);
	}

	/** \brief Queues event as different ids.
	 *
	 * Queues the event as the ids specified by the interval \c begin..end.
	 *
	 * Equivalent to <tt>queue_as(\a begin, \a end, \a strategy, at_back, \a result_callback)</tt>.
	 *
	 * \tparam Iterator The type of the iterators representing the range.
	 *
	 * \param begin The begin of the range of event ids to be added
	 * \param end One past the end of the range of event ids to be added
	 * \param strategy What to do in case of an existing duplicate of the
	 *     given event it:
	 *     - \c discard to not insert the event id if an equivalent event id
	 *            is queued already; discarded will be returned in this
	 *            case, queued otherwise,
	 *     - \c replace to remove any equivalent event id that is queued
	 *            already; replaced will be returned if any event id was
	 *            removed from the queue, queued otherwise
	 *     - \c duplicate to ignore existing equivalent event ids; queuing
	 *            an event with this strategy will always return queued.
	 * \param result_callback A function that takes the iterator to an event id
	 *     as well as the result from attempting to queue this id.
	 */
	template<typename Iterator, typename ResultCallback = std::function<void(Iterator, queuing_result)>>
	inline void queue_as(
		Iterator begin, Iterator end,
		queuing_strategy strategy,
		ResultCallback result_callback
	) {
		queue_as(begin, end, strategy, at_back, result_callback);
	}

	/** \brief Queues event as different ids.
	 *
	 * Queues the event as the ids specified by the interval \c begin..end.
	 *
	 * Equivalent to <tt>queue_as(\a begin, \a end, discard, at_back, \a result_callback)</tt>.
	 *
	 * \tparam Iterator The type of the iterators representing the range.
	 *
	 * \param begin The begin of the range of event ids to be added
	 * \param end One past the end of the range of event ids to be added
	 * \param result_callback A function that takes the iterator to an event id
	 *     as well as the result from attempting to queue this id.
	 */
	template<typename Iterator, typename ResultCallback = std::function<void(Iterator, queuing_result)>>
	inline void queue_as(
		Iterator begin, Iterator end,
		ResultCallback result_callback
	) {
		queue_as(begin, end, discard, at_back, result_callback);
	}

	/** \brief Removes event ids from the queue.
	 *
	 * Removes all event ids equivalent to the given id from the queue.
	 *
	 * \param id The event id to remove from the queue.
	 *
	 * \return <tt>true</tt> if any ids have been removed from the queue,
	 *         <tt>false</tt> otherwise
	 */
	bool unqueue(id_type id);

	/** \brief Removes event ids from the queue.
	 *
	 * Removes all event ids fulfilling (yielding <tt>true</tt> the given matcher.
	 *
	 * \param matcher The function to match the event ids against.
	 *
	 * \return <tt>true</tt> if any ids have been removed from the queue,
	 *         <tt>false</tt> otherwise
	 */
	bool unqueue(id_type::matcher matcher);

	/** \brief Checks whether this event is queued as a specific event id.
	 *
	 * \param id The id to check for.
	 *
	 * \return <tt>true</tt> if the event is queued as the given event id,
	 *         <tt>false</tt> otherwise
	 */
	bool is_queued_as(id_type id) const;

	/** \brief Checks whether this event is queued as an id meeting certain criteria.
	 *
	 * Checks whether this event is queued as any event id that fulfills
	 * (returns <tt>true</tt>) the given matcher.
	 *
	 * \param matcher The function to match the event ids against.
	 *
	 * \return <tt>true</tt> if the event is queued as any matching event id,
	 *         <tt>false</tt> otherwise
	 *
	 * \note This function will return after the first positive match. You can
	 *       use a matcher that always returns false (and discard the return
	 *       value of this function) to make more detailed inquiries about the
	 *       currently queued event ids.
	 */
	bool is_queued_as(id_type::matcher matcher) const;

	/** \brief Queues this event to its irc contexts main event queue.
	 */
	void queue();

	/** \brief Registers another event to be queued right after this one.
	 *
	 * Events registered using this function will be handled before the next
	 * event of the main queue.
	 */
	void afterwards(pointer);

	/** \brief Pops the next event id from the queue.
	 *
	 * \return The next event id this event is queued as or an invalid event id
	 *         if the queue is empty.
	 *
	 * \note This function is intended to be used by the event manager handling
	 *       this event. Use by other entities may result in unexpected behavior.
	 */
	id_type pop_next_queued_id();

private:
	queuing_result prepare_append_queue(id_queue_type&, id_type, queuing_strategy);
	void append_to_queue_unchecked(id_queue_type&, queuing_position);
};

}

namespace std {
	/** \brief Specialization of std::less for slirc::event::id_type
	 */
	template<>
	struct less<::slirc::event::id_type> {
		/** \brief Compares two event ids.
		 *
		 * \param lhs the first event id
		 * \param rhs the second event id
		 *
		 * \return
		 *     - <tt>true</tt> if lhs is ordered before rhs,
		 *     - <tt>false</tt> otherwise
		 */
		bool operator()(
			const ::slirc::event::id_type &lhs,
			const ::slirc::event::id_type &rhs
		) const {
			return (lhs.index != rhs.index)
				? std::less<decltype(lhs.index)>()(lhs.index, rhs.index)
				: std::less<decltype(lhs.id)>()(lhs.id, rhs.id);
		}
	};

	/** \brief Specialization of std::hash for slirc::event::id_type
	 */
	template<>
	struct hash<::slirc::event::id_type> {
		/** \brief Hashes an event id.
		 *
		 * \param id The id to hash.
		 *
		 * \return The hash for id.
		 */
		std::size_t operator()(
			const ::slirc::event::id_type &id
		) const {
			if (!id.index) return 0;
			return
				std::hash<std::type_index>()(*id.index) ^
				std::hash<slirc::event::underlying_id_type>()(id.id);
		}
	};
}

#endif // SLIRC_EVENT_HPP_INCLUDED
