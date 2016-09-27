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

#include "../../include/slirc/modules/event_manager.hpp"

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <iterator>
#include <memory>
#include <mutex>
#include <unordered_map>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#	include <boost/signals2.hpp>
#pragma GCC diagnostic pop

#include "../../include/slirc/event.hpp"

namespace {
	struct disconnect_signals2 {
		boost::signals2::connection conn;

		disconnect_signals2(const boost::signals2::connection &conn_)
		: conn(conn_) {}

		void operator()(slirc::apis::event_manager*) const {
			conn.disconnect();
		}

		bool operator<(const disconnect_signals2 &other) const {
			return conn < other.conn;
		}
	};

	void try_unqueue(
		std::deque<slirc::event::pointer> &queue,
		std::vector<slirc::apis::event_manager::event_consumer_type> &queue_consumers,
		std::vector<slirc::apis::event_manager::event_consumer_type>::size_type &queue_consumer_index
	) {
		// requires: mutex for variables is locked!
		while(!queue.empty() && queue_consumer_index < queue_consumers.size()) {
			if (queue_consumers[queue_consumer_index++](queue.front())) {
				// event is taken and being handled
				queue.pop_front();
			}
		}

		if (queue_consumer_index && queue_consumer_index == queue_consumers.size()) {
			queue_consumers.resize(queue_consumer_index = 0);
		}
	}

	struct returning_wait_event_consumer_data {
		std::mutex mutex;
		std::condition_variable condvar;
		bool awaits_event;
		slirc::event::pointer &event;

		returning_wait_event_consumer_data(slirc::event::pointer &event_)
		: mutex()
		, condvar()
		, awaits_event(true)
		, event(event_) {}
	};

	std::shared_ptr<returning_wait_event_consumer_data> prepare_returning_wait_event_data(slirc::event::pointer &event) {
		return std::make_shared<returning_wait_event_consumer_data>(event);
	}

	slirc::apis::event_manager::event_consumer_type make_returning_wait_event_consumer(std::shared_ptr<returning_wait_event_consumer_data> data) {
		std::weak_ptr<returning_wait_event_consumer_data> weak_data = data;
		return [weak_data](slirc::event::pointer event) {
			std::shared_ptr<returning_wait_event_consumer_data> data = weak_data.lock();
			if (data) {
				std::unique_lock<std::mutex> lock(data->mutex);
				if (data->awaits_event) {
					SLIRC_ASSERT( !data->event && "Consumer structure may not contain an event already is awaits_event is still true." );
					data->event = event;
					data->awaits_event = false;
					data->condvar.notify_all();
					return true;
				}
			}
			return false;
		};
	}
}

struct slirc::modules::event_manager::impl {
	std::unordered_map<event::id_type, boost::signals2::signal<void(event::pointer)>> signals;

	std::mutex queue_mutex;
	/* ^ */ std::deque<event::pointer> queue;
	/* ^ */ std::vector<event_consumer_type> queue_consumers;
	/* ^ */ std::vector<event_consumer_type>::size_type queue_consumer_index;

	impl()
	: signals()
	, queue_mutex()
	, queue()
	, queue_consumers()
	, queue_consumer_index(0) {}
};

slirc::modules::event_manager::event_manager(slirc::irc &irc_)
: apis::event_manager(irc_)
, impl_(new impl) {}

slirc::apis::event_manager::connection slirc::modules::event_manager::connect(
	event::id_type event_id,
	handler_type handler,
	connection_priority priority
) {
	return make_connection(disconnect_signals2(
		impl_->signals[event_id].connect(
			static_cast<std::underlying_type<connection_priority>::type>(priority),
			handler
		)
	));
}

void slirc::modules::event_manager::handle(event::pointer e) {
	e->handle_as(events::begin_handling);

	event::id_type next_id = e->pop_next_queued_id();
	do {
		while(next_id) {
			e->handle_as(next_id);
			next_id = e->pop_next_queued_id();
		}
		e->handle_as(events::finishing_handling);

		// check again; possibly finishing_handling has added new events?
		next_id = e->pop_next_queued_id();
	} while(next_id);

	e->handle_as(events::finished_handling);

	handle_afterwards *ha = e->components.find<handle_afterwards>();
	if (ha) {
		{ std::unique_lock<std::mutex> lock(impl_->queue_mutex);
			std::copy(
				ha->events.rbegin(), ha->events.rend(),
				std::front_inserter(impl_->queue)
			);
			try_unqueue(impl_->queue, impl_->queue_consumers, impl_->queue_consumer_index);
		}
		e->components.remove<handle_afterwards>();
	}
}

void slirc::modules::event_manager::handle_as(event::pointer e) {
	impl_->signals[e->current_id](e);
}



void slirc::modules::event_manager::queue(event::pointer e) {
	{ std::unique_lock<std::mutex> queue_lock(impl_->queue_mutex);
		impl_->queue.push_back(e);
		try_unqueue(impl_->queue, impl_->queue_consumers, impl_->queue_consumer_index);
	}
}

slirc::event::pointer slirc::modules::event_manager::wait_event() {
	event::pointer ep;

	{ std::unique_lock<std::mutex> queue_lock(impl_->queue_mutex);
		if (!impl_->queue.empty()) {
			ep = impl_->queue.front();
			impl_->queue.pop_front();
			return ep;
		}
	}

	auto data = prepare_returning_wait_event_data(ep);
	{ std::unique_lock<std::mutex> data_lock(data->mutex);
		{ std::unique_lock<std::mutex> queue_lock(impl_->queue_mutex);
			if (!impl_->queue.empty()) {
				ep = impl_->queue.front();
				impl_->queue.pop_front();
				return ep;
			}
			impl_->queue_consumers.push_back(make_returning_wait_event_consumer(data));
		}

		data->condvar.wait(data_lock, [data](){ return !data->awaits_event; });
	}

	return ep;
}

slirc::event::pointer slirc::modules::event_manager::wait_event(std::chrono::milliseconds timeout) {
	event::pointer ep;

	{ std::unique_lock<std::mutex> queue_lock(impl_->queue_mutex);
		if (!impl_->queue.empty()) {
			ep = impl_->queue.front();
			impl_->queue.pop_front();
			return ep;
		}
	}

	auto data = prepare_returning_wait_event_data(ep);
	{ std::unique_lock<std::mutex> data_lock(data->mutex);
		{ std::unique_lock<std::mutex> queue_lock(impl_->queue_mutex);
			if (!impl_->queue.empty()) {
				ep = impl_->queue.front();
				impl_->queue.pop_front();
				return ep;
			}
			impl_->queue_consumers.push_back(make_returning_wait_event_consumer(data));
		}

		data->condvar.wait_for(data_lock, timeout, [data](){ return !data->awaits_event; });

		// avoid falsely "accepting" an event in a race condition
		// if the consumer has locked the data structure already
		data->awaits_event = false;
	}

	return ep;
}

void slirc::modules::event_manager::wait_event(event_consumer_type callback) {
	{ std::unique_lock<std::mutex> lock(impl_->queue_mutex);
		if (!impl_->queue.empty()) {
			if (callback(impl_->queue.front())) {
				impl_->queue.pop_front();
			}
		}
		else {
			impl_->queue_consumers.push_back(callback);
		}
	}
}

bool slirc::modules::event_manager::connection_less(
	const disconnector_type &lhs,
	const disconnector_type &rhs
) {
	const disconnect_signals2 *lhs_ = lhs.target<disconnect_signals2>();
	SLIRC_ASSERT(lhs_ && "Disconnectors given to us for comparison must be of our own connector type!");

	const disconnect_signals2 *rhs_ = rhs.target<disconnect_signals2>();
	SLIRC_ASSERT(rhs_ && "Disconnectors given to us for comparison must be of our own connector type!");

	return *lhs_ < *rhs_;
}
