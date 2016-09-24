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
#include <iomanip>
#include <iostream>
#include <type_traits>

#include "../include/slirc/event.hpp"
#include "../src/event.cpp"

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



enum valid_id_type_1: slirc::event::underlying_id_type { valid_id_1a, valid_id_1b };
enum valid_id_type_2: slirc::event::underlying_id_type { valid_id_2 };

SLIRC_REGISTER_EVENT_ID_ENUM(valid_id_type_1);
SLIRC_REGISTER_EVENT_ID_ENUM(valid_id_type_2);

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
				REQUIRE( e->original_id = valid_id_1a );
			}
		}
	}
}

/*
SLIRCAPI void slirc::event::handle();
SLIRCAPI void slirc::event::handle_as(id_type id);

SLIRCAPI slirc::event::queuing_result slirc::event::queue_as(
	id_type id, queuing_strategy strategy, queuing_position position
);

SLIRCAPI bool slirc::event::unqueue(id_type id);
SLIRCAPI bool slirc::event::unqueue(id_type::matcher matcher);

SLIRCAPI bool slirc::event::is_queued_as(id_type id);
SLIRCAPI bool slirc::event::is_queued_as(id_type::matcher matcher);

SLIRCAPI slirc::event::id_type slirc::event::pop_next_queued_id();
*/
