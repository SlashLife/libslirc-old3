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

#ifndef SLIRC_APIS_EVENT_QUEUE_HPP_INCLUDED
#define SLIRC_APIS_EVENT_QUEUE_HPP_INCLUDED

#include <functional>

#include "../detail/system.hpp"

#include "../event.hpp"
#include "../module.hpp"

namespace slirc {
namespace apis {

/** \brief Defines the interface for the main event queue.
 *
 */
struct SLIRCAPI event_queue: module<event_queue> {
	using module::module;

	struct connection {
	private:
		typedef std::function<void(event_queue*)> disconnector_type;
		disconnector_type disconnector;
		event_queue *equeue;

		connection()=delete;
		connection(disconnector_type discon, event_queue *queue)
		: disconnector(discon)
		, equeue(queue) {}

	public:
		friend struct ::slirc::apis::event_queue;

		connection(const connection &)=default;
		connection &operator=(const connection &)=default;

		inline event_queue *get_queue() const {
			return equeue;
		}

		inline bool connected() const {
			return equeue;
		}

		inline void disconnect() {
			if (equeue) {
				disconnector(equeue);
				equeue = nullptr;
				disconnector = disconnector_type();
			}
		}

		inline bool operator!=(const connection &other) const {
			return (equeue != other.equeue) || (equeue && (
				equeue->connection_less(*this, other) ||
				equeue->connection_less(other, *this)
			));
		}

		inline bool operator==(const connection &other) const {
			return !(*this != other);
		}
	};

	virtual void handle(event::pointer) = 0;
	virtual void handle_as(event::pointer) = 0;

protected:
	inline connection make_connection(connection::disconnector_type disconnector) const {
		return connection(disconnector, const_cast<event_queue*>(this));
	}
	virtual bool connection_less(const connection &lhs, const connection &rhs) = 0;
};

}
}

namespace std {
	template<>
	struct less<::slirc::apis::event_queue::connection> {
		bool operator(
			const ::slirc::apis::event_queue::connection &lhs,
			const ::slirc::apis::event_queue::connection &rhs
		) const {
			return (lhs.equeue != rhs.equeue)
				? std::less<decltype(lhs.equeue)>(lhs.equeue, rhs.equeue)
				: (lhs.equeue && lhs.equeue->connection_less(lhs, rhs));
		}
	};
}

#endif // SLIRC_APIS_EVENT_QUEUE_HPP_INCLUDED
