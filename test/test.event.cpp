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

#include "testcase.hpp"

#include <functional>
#include <iterator>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <vector>

#include "../include/slirc/irc.hpp"
#include "../include/slirc/event.hpp"
#include "../include/slirc/module.hpp"

#include "../src/irc.cpp"
#include "../src/event.cpp"
#include "../src/modules/event_manager.cpp"

namespace slirc { namespace test {
	struct test_overrides {
		static decltype(std::declval<slirc::event::id_type>().id) get_underlying(slirc::event::id_type id) {
			return id.id;
		}
		static decltype(std::declval<slirc::event::id_type>().index) get_enumtype(slirc::event::id_type id) {
			return id.index;
		}
	};
}}

using idlist = std::vector<slirc::event::id_type>;
inline idlist get_queue(slirc::event::pointer ep) {
	idlist container;
	ep->is_queued_as(
		[&](slirc::event::id_type id) -> bool {
			container.push_back(id);
			return false; // always return false to traverse the whole queue
		}
	);
	return container;
}
inline std::ostream &print_queue(std::ostream &os, slirc::event::pointer ep) {
	bool begin=true;
	os << '[';
	ep->is_queued_as(
		[&](slirc::event::id_type id) -> bool {
			if (!begin) os << ", ";
			begin = false;
			id.print_debug(os);
			return false; // always return false to traverse the whole queue
		}
	);
	os << ']';
	return os;
}



typedef signed char wrong_underlying_type;
static_assert(!std::is_same<wrong_underlying_type, slirc::event::underlying_id_type>::value,
	"Testing is useless if wrong base type equals correct base type!");



enum type_test_1: wrong_underlying_type {
	wrong_underlying_type_and_not_registered
};

enum type_test_2: slirc::event::underlying_id_type {
	correct_underlying_type_and_not_registered
};

enum type_test_3: wrong_underlying_type {
	wrong_underlying_type_and_registered
};
// Obsolete: Macro tests for underlying type; replace
// and test with manual definition instead.
// SLIRC_REGISTER_EVENT_ID_ENUM(type_test_3);
constexpr std::true_type slirc_impldetail_enable_as_event_id_type(type_test_3);

enum type_test_4: slirc::event::underlying_id_type {
	correct_underlying_type_and_registered
};
SLIRC_REGISTER_EVENT_ID_ENUM(type_test_4);



enum valid_id_type_1: slirc::event::underlying_id_type { valid_id_1a, valid_id_1b };
enum valid_id_type_2: slirc::event::underlying_id_type { valid_id_2 };
enum valid_id_type_3: slirc::event::underlying_id_type { valid_id_3 };

SLIRC_REGISTER_EVENT_ID_ENUM(valid_id_type_1);
SLIRC_REGISTER_EVENT_ID_ENUM(valid_id_type_2);
SLIRC_REGISTER_EVENT_ID_ENUM(valid_id_type_3);



SCENARIO("event - eligibility of types for event ids (event::is_valid_id_type, event::is_valid_id)", "") {
	GIVEN("a potential event id type") {
		WHEN("it is of the wrong underlying type and not registered as an event id type") {
			THEN("it will not be an eligible event id type") {
				REQUIRE_FALSE( slirc::event::is_valid_id(wrong_underlying_type_and_not_registered) );
			}
		}

		WHEN("it is of the wrong underlying type and registered as an event id type") {
			THEN("it will not be an eligible event id type") {
				REQUIRE_FALSE( slirc::event::is_valid_id(wrong_underlying_type_and_registered) );
			}
		}

		WHEN("it is of the correct underlying type and not registered as an event id type") {
			THEN("it will not be an eligible event id type") {
				REQUIRE_FALSE( slirc::event::is_valid_id(correct_underlying_type_and_not_registered) );
			}
		}

		WHEN("it is of the correct underlying type and registered as an event id type") {
			THEN("it will be an eligible event id type") {
				REQUIRE( slirc::event::is_valid_id(correct_underlying_type_and_registered) );
			}
		}
	}
}



SCENARIO("event - event ids (event::id_type)", "") {
	GIVEN("an event id") {
		slirc::event::id_type event;

		WHEN("default constructed") {
			THEN("converting it to bool yields false") {
				REQUIRE_FALSE( static_cast<bool>(event) );
			}

			THEN("inverting it yields true") {
				REQUIRE( !event );
			}

			THEN("checking it for any type yields false") {
				REQUIRE_FALSE( event.is_of_type<valid_id_type_1>() );
				REQUIRE_FALSE( event.is_of_type<valid_id_type_2>() );
				REQUIRE_FALSE( event.is_of_type(valid_id_1a) );
				REQUIRE_FALSE( event.is_of_type(valid_id_1b) );
				REQUIRE_FALSE( event.is_of_type(valid_id_2) );
			}

			THEN("attempting to get its value fails") {
				REQUIRE_THROWS_AS( event.get<valid_id_type_1>(), std::bad_cast );
				REQUIRE_THROWS_AS( event.get<valid_id_type_2>(), std::bad_cast );
			}
		}

		event = valid_id_1a;
		WHEN("set to an event") {
			THEN("converting it to bool yields true") {
				REQUIRE( static_cast<bool>(event) );
			}

			THEN("inverting it yields false") {
				REQUIRE_FALSE( !event );
			}

			THEN("checking it for an different type yields false") {
				REQUIRE_FALSE( event.is_of_type<valid_id_type_2>() );
				REQUIRE_FALSE( event.is_of_type(valid_id_2) );
			}

			THEN("checking it for the same type yields true") {
				REQUIRE( event.is_of_type<valid_id_type_1>() );
				REQUIRE( event.is_of_type(valid_id_1a) );
				REQUIRE( event.is_of_type(valid_id_1b) );
			}

			THEN("attempting to get its value using a different type fails") {
				REQUIRE_THROWS_AS( event.get<valid_id_type_2>(), std::bad_cast );
			}

			THEN("attempting to get its value using the same type yields the value it was initialized with") {
				REQUIRE( event.get<valid_id_type_1>() == valid_id_1a );
			}
		}
	}

	GIVEN("two event ids") {
		slirc::event::id_type
			event1,
			event2;

		WHEN("both are default constructed") {
			THEN("they compare equal") {
				REQUIRE( event1 == event2 );
			}

			THEN("they do not compare unequal") {
				REQUIRE_FALSE( event1 != event2 );
			}

			THEN("neither compares less than the other") {
				std::less<slirc::event::id_type> less;
				REQUIRE_FALSE( less(event1, event2) );
				REQUIRE_FALSE( less(event2, event1) );
			}
		}

		WHEN("both are set to the same event") {
			event1 = valid_id_1a;
			event2 = valid_id_1a;

			THEN("they compare equal to each other") {
				REQUIRE( event1 == event2 );
			}

			THEN("they compare equal to the value that initialized them") {
				REQUIRE( event1 == valid_id_1a );
				REQUIRE( valid_id_1a == event2 );
			}

			THEN("they do not compare unequal to each other") {
				REQUIRE_FALSE( event1 != event2 );
			}

			THEN("they do not compare unequal to the value that initialized them") {
				REQUIRE_FALSE( event1 != valid_id_1a );
				REQUIRE_FALSE( valid_id_1a != event2 );
			}

			THEN("neither compares less than the other") {
				std::less<slirc::event::id_type> less;
				REQUIRE_FALSE( less(event1, event2) );
				REQUIRE_FALSE( less(event2, event1) );
			}
		}
	}

	GIVEN("two event ids") {
		slirc::event::id_type
			event1 = valid_id_1a,
			event2 = event1;

		WHEN("one is initialized to an event and the other is copy initialized from the first one") {
			event1 = valid_id_1a;
			event2 = valid_id_1a;

			THEN("they compare equal to each other") {
				REQUIRE( event1 == event2 );
			}

			THEN("they compare equal to each other") {
				REQUIRE( event1 == event2 );
			}

			THEN("they compare equal to the value that initialized them") {
				REQUIRE( event1 == valid_id_1a );
				REQUIRE( valid_id_1a == event2 );
			}

			THEN("they do not compare unequal to each other") {
				REQUIRE_FALSE( event1 != event2 );
			}

			THEN("they do not compare unequal to the value that initialized them") {
				REQUIRE_FALSE( event1 != valid_id_1a );
				REQUIRE_FALSE( valid_id_1a != event2 );
			}

			THEN("neither compares less than the other") {
				std::less<slirc::event::id_type> less;
				REQUIRE_FALSE( less(event1, event2) );
				REQUIRE_FALSE( less(event2, event1) );
			}
		}
	}

	GIVEN("two different valid id types and an empty id") {
		slirc::event::id_type
			empty,
			valid1(valid_id_1a),
			valid2(valid_id_2);

		WHEN("comparing any two of them") {
			THEN("they will pairwise not compare equal") {
				REQUIRE_FALSE( empty  == valid1 );
				REQUIRE_FALSE( empty  == valid2 );
				REQUIRE_FALSE( valid1 == valid2 );
			}

			THEN("they will pairwise compare unequal") {
				REQUIRE( empty  != valid1 );
				REQUIRE( empty  != valid2 );
				REQUIRE( valid1 != valid2 );
			}
		}

		WHEN("checking their order using std::less<id_type>") {
			std::less<slirc::event::id_type> less;

			THEN("they will pairwise compare less than in exactly one direction") {
				REQUIRE( less(empty , valid1) != less(valid1, empty ) );
				REQUIRE( less(empty , valid2) != less(valid2, empty ) );
				REQUIRE( less(valid1, valid2) != less(valid2, valid1) );
			}

			THEN("they will not compare in a circle") {
				if (less(empty, valid1) && less(valid1, valid2)) {
					REQUIRE( less(empty, valid2) );
				}
				else if (less(empty, valid2) && less(valid2, valid1)) {
					REQUIRE( less(empty, valid1) );
				}
				else if (less(valid1, empty) && less(empty, valid2)) {
					REQUIRE( less(valid1, valid2) );
				}
				else if (less(valid1, valid2) && less(valid2, empty)) {
					REQUIRE( less(valid1, empty) );
				}
				else if (less(valid2, empty) && less(empty, valid1)) {
					REQUIRE( less(valid2, valid1) );
				}
				else if (less(valid2, valid1) && less(valid1, empty)) {
					REQUIRE( less(valid2, empty) );
				}
				else {
					std::cerr << std::boolalpha;
					std::cerr
						<< "less(empty , valid1) == " << less(empty, valid1) << "\n"
						<< "less(empty , valid1) == " << less(empty, valid2) << "\n"
						<< "less(valid1, valid2) == " << less(valid1, valid2) << "\n";
					std::cerr.flush();

					FAIL("event ids are not in any of the possible orders (test case broken?)");
				}
			}
		}
	}

	GIVEN("two different valid ids of the same type and an empty id") {
		slirc::event::id_type
			empty,
			valid1(valid_id_1a),
			valid2(valid_id_2);

		WHEN("comparing any two of them") {
			THEN("they will pairwise not compare equal") {
				REQUIRE_FALSE( empty  == valid1 );
				REQUIRE_FALSE( empty  == valid2 );
				REQUIRE_FALSE( valid1 == valid2 );
			}

			THEN("they will pairwise compare unequal") {
				REQUIRE( empty  != valid1 );
				REQUIRE( empty  != valid2 );
				REQUIRE( valid1 != valid2 );
			}
		}

		WHEN("checking their order using std::less<id_type>") {
			std::less<slirc::event::id_type> less;

			THEN("they will pairwise compare less than in exactly one direction") {
				REQUIRE( less(empty , valid1) != less(valid1, empty ) );
				REQUIRE( less(empty , valid2) != less(valid2, empty ) );
				REQUIRE( less(valid1, valid2) != less(valid2, valid1) );
			}

			THEN("they will not compare in a circle") {
				if (less(empty, valid1) && less(valid1, valid2)) {
					REQUIRE( less(empty, valid2) );
				}
				else if (less(empty, valid2) && less(valid2, valid1)) {
					REQUIRE( less(empty, valid1) );
				}
				else if (less(valid1, empty) && less(empty, valid2)) {
					REQUIRE( less(valid1, valid2) );
				}
				else if (less(valid1, valid2) && less(valid2, empty)) {
					REQUIRE( less(valid1, empty) );
				}
				else if (less(valid2, empty) && less(empty, valid1)) {
					REQUIRE( less(valid2, valid1) );
				}
				else if (less(valid2, valid1) && less(valid1, empty)) {
					REQUIRE( less(valid2, empty) );
				}
				else {
					std::cerr << std::boolalpha;
					std::cerr
						<< "less(empty , valid1) == " << less(empty, valid1) << "\n"
						<< "less(empty , valid1) == " << less(empty, valid2) << "\n"
						<< "less(valid1, valid2) == " << less(valid1, valid2) << "\n";
					std::cerr.flush();

					FAIL("event ids are not in any of the possible orders (test case broken?)");
				}
			}
		}
	}
}

SCENARIO("event - event::handle(), event::handle_as()", "") {
	GIVEN("an irc context and an event") {
		slirc::irc irc;
		auto e = irc.make_event(valid_id_1a);

		WHEN("checking the original event id") {
			THEN("it will be the same as the one it was initialized as.") {
				REQUIRE( e->original_id == valid_id_1a );
			}
		}

		e->pop_next_queued_id();
	}
}

SCENARIO("event - event queue, basic queuing", "") {
	GIVEN("an event") {
		slirc::irc irc;
		auto e = irc.make_event(valid_id_1a);

		WHEN("freshly created") {
			THEN("it contains only the id it was created with") {
				REQUIRE( get_queue(e) == idlist{ valid_id_1a } );
			}
		}

		WHEN("queueing a different event") {
			THEN("the other event cannot be found before being added") {
				REQUIRE_FALSE( e->is_queued_as( valid_id_1b ) );
			}

			e->queue_as( valid_id_1b );

			THEN("the other event can be found after being added") {
				REQUIRE( e->is_queued_as( valid_id_1b ) );
			}

			THEN("the new event is by default appended to the end") {
				REQUIRE( get_queue(e) == (idlist{valid_id_1a, valid_id_1b}) );
			}
		}

		WHEN("queueing a different event at the beginning") {
			THEN("the other event cannot be found before being added") {
				REQUIRE_FALSE( e->is_queued_as( valid_id_1b ) );
			}

			e->queue_as( valid_id_1b, slirc::event::at_front );

			THEN("the other event can be found after being added") {
				REQUIRE( e->is_queued_as( valid_id_1b ) );
			}

			THEN("the new event is queued at the beginning") {
				REQUIRE( get_queue(e) == (idlist{valid_id_1b, valid_id_1a}) );
			}
		}
	}

	GIVEN("an event containing one event id once and another one twice") {
		slirc::irc irc;
		auto e = irc.make_event(valid_id_1a);

		e->queue_as( valid_id_1b, slirc::event::duplicate, slirc::event::at_front );
		e->queue_as( valid_id_1b, slirc::event::duplicate, slirc::event::at_back );

		REQUIRE( get_queue(e) == (idlist{valid_id_1b, valid_id_1a, valid_id_1b}) );

		WHEN("removing the single event id") {
			REQUIRE( e->unqueue(valid_id_1a) );

			THEN("the duplicated event ids will stay in the queue") {
				REQUIRE( get_queue(e) == (idlist{valid_id_1b, valid_id_1b}) );
			}
		}

		WHEN("removing the duplicate event id") {
			REQUIRE( e->unqueue(valid_id_1b) );

			THEN("the single event id will stay in the queue") {
				REQUIRE( get_queue(e) == (idlist{valid_id_1a}) );
			}
		}
	}
}

TEST_CASE("event - event queue, complex operations", "") {
	slirc::irc irc;
	auto e = irc.make_event(valid_id_1a);
	e->queue_as(valid_id_1b, slirc::event::duplicate);
	e->queue_as(valid_id_1a, slirc::event::duplicate);
	e->queue_as(valid_id_2, slirc::event::duplicate);
	REQUIRE( get_queue(e) == (idlist{valid_id_1a, valid_id_1b, valid_id_1a, valid_id_2}) );

	SECTION("queue_as (single param) - at_back, discard, existing element") {
		REQUIRE( slirc::event::discarded == e->queue_as(valid_id_1a, slirc::event::discard, slirc::event::at_back) );
		REQUIRE( get_queue(e) == (idlist{valid_id_1a, valid_id_1b, valid_id_1a, valid_id_2}) );
	}

	SECTION("queue_as (single param) - at_back, discard, non-existing element") {
		REQUIRE( slirc::event::queued == e->queue_as(valid_id_3, slirc::event::discard, slirc::event::at_back) );
		REQUIRE( get_queue(e) == (idlist{valid_id_1a, valid_id_1b, valid_id_1a, valid_id_2, valid_id_3}) );
	}

	SECTION("queue_as (single param) - at_front, duplicate, existing element") {
		REQUIRE( slirc::event::queued == e->queue_as(valid_id_1a, slirc::event::duplicate, slirc::event::at_front) );
		REQUIRE( get_queue(e) == (idlist{valid_id_1a, valid_id_1a, valid_id_1b, valid_id_1a, valid_id_2}) );
	}

	SECTION("queue_as (single param) - at_front, duplicate, non-existing element") {
		REQUIRE( slirc::event::queued == e->queue_as(valid_id_3, slirc::event::duplicate, slirc::event::at_front) );
		REQUIRE( get_queue(e) == (idlist{valid_id_3, valid_id_1a, valid_id_1b, valid_id_1a, valid_id_2}) );
	}

	SECTION("queue_as (single param) - at_back, replace, existing element") {
		REQUIRE( slirc::event::replaced == e->queue_as(valid_id_1a, slirc::event::replace, slirc::event::at_back) );
		REQUIRE( get_queue(e) == (idlist{valid_id_1b, valid_id_2, valid_id_1a}) );
	}

	SECTION("queue_as (single param) - at_back, replace, non-existing element") {
		REQUIRE( slirc::event::queued == e->queue_as(valid_id_3, slirc::event::replace, slirc::event::at_back) );
		REQUIRE( get_queue(e) == (idlist{valid_id_1a, valid_id_1b, valid_id_1a, valid_id_2, valid_id_3}) );
	}



	typedef std::vector<slirc::event::queuing_result> result_type;
	std::vector<slirc::event::id_type> new_ids = {
		valid_id_1a, slirc::event::id_type{}, valid_id_1a, valid_id_1b, valid_id_3 };
	result_type results;
	const auto result_logger = [&](auto, auto value){ results.push_back(value); };

	SECTION("queue_as (multiple param) - at_back, discard, existing element") {
		e->queue_as(new_ids.begin(), new_ids.end(), slirc::event::discard, slirc::event::at_back, result_logger);
		REQUIRE( get_queue(e) == (idlist{valid_id_1a, valid_id_1b, valid_id_1a, valid_id_2, valid_id_3}) );
		REQUIRE( results == (result_type{slirc::event::discarded,
			slirc::event::invalid, slirc::event::discarded,
			slirc::event::discarded, slirc::event::queued}) );
	}

	SECTION("queue_as (multiple param) - at_front, duplicate, existing element") {
		e->queue_as(new_ids.begin(), new_ids.end(), slirc::event::duplicate, slirc::event::at_front, result_logger);
		REQUIRE( get_queue(e) == (idlist{valid_id_1a, valid_id_1a, valid_id_1b,
			valid_id_3, valid_id_1a, valid_id_1b, valid_id_1a, valid_id_2}) );
		REQUIRE( results == (result_type{slirc::event::queued,
			slirc::event::invalid, slirc::event::queued,
			slirc::event::queued, slirc::event::queued}) );
	}

	SECTION("queue_as (multiple param) - at_back, replace, existing element") {
		e->queue_as(new_ids.begin(), new_ids.end(), slirc::event::replace, slirc::event::at_back, result_logger);
		REQUIRE( get_queue(e) == (idlist{valid_id_2, valid_id_1a, valid_id_1a, valid_id_1b, valid_id_3}) );
		REQUIRE( results == (result_type{slirc::event::replaced,
			slirc::event::invalid, slirc::event::queued,
			slirc::event::replaced, slirc::event::queued}) );
	}



	SECTION("unqueue (single id & matcher), is_queued_as (single id)") {
		unsigned nth;
		auto remove_first = [&nth](auto){ return 0==nth++; };

		REQUIRE( e->is_queued_as(valid_id_1a) );
		REQUIRE( get_queue(e) == (idlist{valid_id_1a, valid_id_1b, valid_id_1a, valid_id_2}) );
		REQUIRE( e->unqueue(valid_id_1a) );
		REQUIRE( get_queue(e) == (idlist{valid_id_1b, valid_id_2}) );
		REQUIRE_FALSE( e->unqueue(valid_id_1a) );
		REQUIRE( get_queue(e) == (idlist{valid_id_1b, valid_id_2}) );
		REQUIRE_FALSE( e->is_queued_as(valid_id_1a) );

		REQUIRE( e->is_queued_as(valid_id_1b) );
		nth=0;
		REQUIRE( e->unqueue(remove_first) );
		REQUIRE( get_queue(e) == (idlist{valid_id_2}) );
		REQUIRE_FALSE( e->is_queued_as(valid_id_1b) );

		REQUIRE( e->is_queued_as(valid_id_2) );
		nth=0;
		REQUIRE( e->unqueue(remove_first) );
		REQUIRE( get_queue(e) == (idlist{}) );
		REQUIRE_FALSE( e->is_queued_as(valid_id_2) );

		nth=0;
		REQUIRE_FALSE( e->unqueue(remove_first) );
		REQUIRE( get_queue(e) == (idlist{}) );
	}



	SECTION("is_queued_as (matcher)") {
		auto all_false = [](auto){ return false; };
		REQUIRE_FALSE( e->is_queued_as(all_false) );

		const int nth=3;
		REQUIRE( nth < get_queue(e).size() );
		int num_checked=0;
		auto accept_nth = [&](auto){ return nth == ++num_checked; };
		REQUIRE( e->is_queued_as(accept_nth) );
		REQUIRE( nth == num_checked );
	}



	SECTION("pop_next_queued_id") {
		REQUIRE( valid_id_1a == e->pop_next_queued_id() );
		REQUIRE( valid_id_1b == e->pop_next_queued_id() );
		REQUIRE( valid_id_1a == e->pop_next_queued_id() );
		REQUIRE( valid_id_2 == e->pop_next_queued_id() );
		REQUIRE( !e->pop_next_queued_id() );
	}
}
