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

#include "../include/slirc/component.hpp"
#include "../include/slirc/irc.hpp"
#include "../include/slirc/module.hpp"
#include "../include/slirc/apis/event_manager.hpp"

#include "../src/event.cpp"
#include "../src/irc.cpp"
#include "../src/modules/event_manager.cpp"

struct my_component: slirc::component<my_component> {};

SCENARIO("irc - components", "") {
	GIVEN("an irc context") {
		slirc::irc irc;

		WHEN("putting a component into it") {
			irc.components.insert(my_component());

			THEN("the irc context contains that component") {
				REQUIRE( irc.components.has<my_component>() );
			}
		}
	}
}



struct module_base: slirc::module<module_base> { module_base(slirc::irc &irc_): slirc::module<module_base>(irc_) {} };
struct module_derived: module_base { using module_base::module_base; };
struct module_derived_2: module_base { using module_base::module_base; };

struct module_track_lifetime: slirc::module<module_track_lifetime> {
	enum state { uninitialized, constructed, destructed };
	state &st;

	module_track_lifetime(slirc::irc &irc_, state &st_)
	: slirc::module<module_track_lifetime>(irc_)
	, st(st_) {
		st = constructed;
	}

	~module_track_lifetime() {
		st = destructed;
	}
};

SCENARIO("irc - module API", "") {
	GIVEN("an irc context") {
		slirc::irc irc;

		WHEN("nothing has happened yet") {
			THEN("none of the modules should be loaded") {
				REQUIRE_FALSE( irc.find<module_track_lifetime>() );
				REQUIRE_FALSE( irc.find<module_base>() );
				REQUIRE_FALSE( irc.find<module_derived>() );

				REQUIRE_THROWS_AS( irc.get<module_track_lifetime>(), std::range_error );
				REQUIRE_THROWS_AS( irc.get<module_base>(), std::range_error );
				REQUIRE_THROWS_AS( irc.get<module_derived>(), std::range_error );
			}

			THEN("attempting to unload any module should return false") {
				REQUIRE_FALSE( irc.unload<module_track_lifetime>() );
				REQUIRE_FALSE( irc.unload<module_base>() );
				REQUIRE_FALSE( irc.unload<module_derived>() );
			}
		}

		WHEN("loading the base version of a polymorphic module") {
			irc.load<module_base>();

			THEN("the loaded module can be found") {
				REQUIRE( irc.find<module_base>() );
				REQUIRE_NOTHROW( irc.get<module_base>() );
			}

			THEN("a derived module and an unrelated module can not be found") {
				REQUIRE_FALSE( irc.find<module_track_lifetime>() );
				REQUIRE_FALSE( irc.find<module_derived>() );

				REQUIRE_THROWS_AS( irc.get<module_track_lifetime>(), std::range_error );
				REQUIRE_THROWS_AS( irc.get<module_derived>(), slirc::exceptions::module_conflict );
			}

			THEN("attemping to unload an unrelated module will fail returning false") {
				REQUIRE_FALSE( irc.unload<module_track_lifetime>() );
			}

			THEN("attemping to unload a derived module will fail with an exception") {
				REQUIRE_THROWS_AS( irc.unload<module_derived>(), slirc::exceptions::module_conflict );
			}

			THEN("an unrelated module can be loaded (and then unloaded)") {
				module_track_lifetime::state state = module_track_lifetime::uninitialized;
				REQUIRE_NOTHROW( irc.load<module_track_lifetime>(state) );
				REQUIRE( irc.unload<module_track_lifetime>() );
			}

			THEN("the module can successfully be unloaded again, returning true") {
				REQUIRE( irc.unload<module_base>() );
			}
		}

		WHEN("loading the derived version of a polymorphic module") {
			irc.load<module_derived>();

			THEN("the loaded module can be found") {
				REQUIRE( irc.find<module_derived>() );
				REQUIRE_NOTHROW( irc.get<module_derived>() );
			}

			THEN("the loaded module can also be found using its base type") {
				REQUIRE( irc.find<module_base>() );
				REQUIRE_NOTHROW( irc.get<module_base>() );
			}

			THEN("an unrelated module can not be found") {
				REQUIRE_FALSE( irc.find<module_track_lifetime>() );
				REQUIRE_THROWS_AS( irc.get<module_track_lifetime>(), std::range_error );
			}

			THEN("attemping to unload an unrelated module will fail returning false") {
				REQUIRE_FALSE( irc.unload<module_track_lifetime>() );
			}

			THEN("attemping to unload an unrelated derived module of the same base will fail with an exception") {
				REQUIRE_THROWS_AS( irc.unload<module_derived_2>(), slirc::exceptions::module_conflict );
			}

			THEN("an unrelated module can be loaded (and then unloaded)") {
				module_track_lifetime::state state = module_track_lifetime::uninitialized;
				REQUIRE_NOTHROW( irc.load<module_track_lifetime>(state) );
				REQUIRE( irc.unload<module_track_lifetime>() );
			}

			THEN("the module can successfully be unloaded by its base type, returning true") {
				REQUIRE( irc.unload<module_derived>() );
			}
		}

		WHEN("loading and unloading a module") {
			module_track_lifetime::state state = module_track_lifetime::uninitialized;

			REQUIRE_NOTHROW( irc.load<module_track_lifetime>(state) );
			THEN("while loading a module the module will be constructed") {
				REQUIRE( state == module_track_lifetime::constructed );
			}

			REQUIRE( irc.unload<module_track_lifetime>() );
			THEN("while unloading a module the module will be destructed") {
				REQUIRE( state == module_track_lifetime::destructed );
			}
		}
	}
}



enum test_events {
	test_event
};
SLIRC_REGISTER_EVENT_ID_ENUM(test_events);

SCENARIO("irc - event API", "") {
	GIVEN("an irc context") {
		slirc::irc irc;

		WHEN("default constructed") {

			THEN("it contains an event_manager module") {
				REQUIRE( irc.find<slirc::apis::event_manager>() );
			}

			THEN("the event_manager module can be accessed through the special getter") {
				slirc::apis::event_manager *em = irc.find<slirc::apis::event_manager>();
				REQUIRE( em == &irc.event_manager() );
			}
		}

		WHEN("creating an event") {
			slirc::event::pointer e = irc.make_event(test_event);

			THEN("the event is associated with the IRC context") {
				REQUIRE( &irc == &e->irc );
			}
		}
	}
}
