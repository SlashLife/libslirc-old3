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

#include "../include/slirc/event.hpp"

#include "../include/slirc/irc.hpp"
#include "../include/slirc/util/scoped_swap.hpp"

slirc::event::event(constructor_tag, slirc::irc &irc_, id_type original_id_)
: irc(irc_)
, original_id(original_id_)
, current_id(current_id_mutable)
, current_id_mutable()
, queued_ids()
, next_id_index(0) {
	if (!original_id_) {
		throw exceptions::invalid_event_id();
	}
	queued_ids.push_back(original_id_);
}

void slirc::event::handle() {
	irc.event_manager().handle(shared_from_this());
}

void slirc::event::handle_as(id_type id) {
	util::scoped_swap<id_type> id_swap(current_id_mutable, id);
	irc.event_manager().handle_as(shared_from_this());
}

slirc::event::queuing_result slirc::event::queue_as(
	id_type id, queuing_strategy strategy, queuing_position position
) {
	id_queue_type add_ids;
	queuing_result result = prepare_append_queue(add_ids, id, strategy);
	if (!add_ids.empty()) {
		append_to_queue_unchecked(add_ids, position);
	}
	return result;
}

bool slirc::event::unqueue(id_type id) {
	auto it = std::remove(
		queued_ids.begin(), queued_ids.end(),
		id
	);
	if (it == queued_ids.end()) {
		return false;
	}
	queued_ids.erase(it, queued_ids.end());
	return true;
}

bool slirc::event::unqueue(id_type::matcher matcher) {
	auto it = std::remove_if(
		queued_ids.begin(), queued_ids.end(),
		matcher
	);
	if (it == queued_ids.end()) {
		return false;
	}
	queued_ids.erase(it, queued_ids.end());
	return true;
}

bool slirc::event::is_queued_as(id_type id) const {
	return queued_ids.end() != std::find(
		queued_ids.begin() + next_id_index, queued_ids.end(),
		id
	);
}

bool slirc::event::is_queued_as(id_type::matcher matcher) const {
	return queued_ids.end() != std::find_if(
		queued_ids.begin() + next_id_index, queued_ids.end(),
		matcher
	);
}

slirc::event::id_type slirc::event::pop_next_queued_id() {
	if (next_id_index < queued_ids.size()) {
		return queued_ids[next_id_index++];
	}
	return id_type();
}

slirc::event::queuing_result slirc::event::prepare_append_queue(
	id_queue_type &add_ids, id_type newid, queuing_strategy strategy
) {
	if (!newid) {
		return invalid;
	}

	switch(strategy) {
		case discard: {
			if (
				queued_ids.end() != std::find(
					queued_ids.begin() + next_id_index,
					queued_ids.end(),
					newid
				)
			) {
				return discarded;
			}
			else {
				add_ids.push_back(newid);
				return queued;
			}
			break;
		}

		case duplicate:
		case replace: {
			id_queue_type::iterator new_end = (strategy == replace)
				// remove existing duplicates when replacing ...
				? std::remove(queued_ids.begin() + next_id_index, queued_ids.end(), newid )
				// ... otherwise just skip the upcoming check for erasing from the queue
				: queued_ids.end();
			if (new_end != queued_ids.end()) {
				SLIRC_ASSERT( strategy == replace &&
					"Must not erase elements from existing queue for any other strategy" );
				queued_ids.erase(new_end, queued_ids.end());
				add_ids.push_back(newid);
				return replaced;
			}
			else {
				add_ids.push_back(newid);
				return queued;
			}
			break;
		}

		default: {
			SLIRC_ASSERT( false && "Invalid strategy!" );
			std::terminate();
		}
	}
}

void slirc::event::append_to_queue_unchecked(
	id_queue_type &add_ids, queuing_position position
) {
	// we assume all checks have been done beforehand
	// (semantically, we're operating in a "duplicate" strategy mode)

	if (!add_ids.empty()) {
		if (position == at_front) {
			if (add_ids.size() <= next_id_index) {
				// enough space in front of existing queue
				const auto end_of_insert = std::copy(
					add_ids.begin(), add_ids.end(),
					queued_ids.begin() + next_id_index - add_ids.size()
				);
				((void)end_of_insert); // only used in assertion
				SLIRC_ASSERT( end_of_insert == queued_ids.begin()+next_id_index
					&& "Inserted ID range does not line up with previous ids." );

				next_id_index -= add_ids.size();
			}
			else {
				// not enough space in existing queue;
				// append current queue to new queue and swap
				const auto prev_size = add_ids.size();
				add_ids.resize( add_ids.size() + queued_ids.size() - next_id_index );

				auto end_of_insert = std::copy(
					queued_ids.begin() + next_id_index, queued_ids.end(),
					add_ids.begin() + prev_size
				);
				((void)end_of_insert); // only used in assertion
				SLIRC_ASSERT( end_of_insert == add_ids.end()
					&& "Inserted ID range does not line up with previous ids." );

				std::swap(add_ids, queued_ids);
				next_id_index = 0;
			}
		}
		else {
			SLIRC_ASSERT( position == at_back && "Invalid queue insertion position." );

			if (
				// There is space in front of the queue, ...
				0 < next_id_index &&
				// ... but not enough space in the back to accomodate all
				// elements that are about to be inserted ...
				queued_ids.capacity() - queued_ids.size() < add_ids.size()
			) {
				// ... maybe after reorganizing the original elements to the
				// beginning the space will suffice - if not then at least
				// we reallocate with as few elements to be copied as
				// possible.
				const auto prev_length = queued_ids.size() - next_id_index;
				((void)prev_length); // only used in assertion

				auto new_end = std::copy(
					queued_ids.begin() + next_id_index, queued_ids.end(),
					queued_ids.begin()
				);
				next_id_index = 0;

				queued_ids.resize(new_end - queued_ids.begin());

				SLIRC_ASSERT( queued_ids.size() == prev_length
					&& "Reorganizing the queue wrongfully lost or created elements." );
			}

			// now, with or without reorganizing, append new elements to end
			auto prev_size = queued_ids.size();
			((void)prev_size); // only used in assertion

			queued_ids.insert(queued_ids.end(), add_ids.begin(), add_ids.end());
			SLIRC_ASSERT( queued_ids.size() == prev_size + add_ids.size()
				&& "Appending to the queue wrongfully lost or created elements." );
		}
	}
}
