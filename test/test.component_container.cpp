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
#include "../include/slirc/component_container.hpp"

namespace slirc { namespace test { struct test_overrides {
	static auto &component_containter_contents(slirc::component_container &cc) {
		return cc.contents;
	}
};}}

struct component_a: slirc::component<component_a> {};
struct component_b: slirc::component<component_b> {};

struct component_inherit_base: slirc::component<component_inherit_base> {};
struct component_inherit_derived_a: component_inherit_base {};
struct component_inherit_derived_a_derived: component_inherit_derived_a {};
struct component_inherit_derived_b: component_inherit_base {};

struct not_a_component_hacked_specialization_no_common_base;
struct not_a_component_hacked_specialization_wrong_component_type_typedef;

namespace slirc {
	template<>
	struct component<::not_a_component_hacked_specialization_no_common_base> {
		typedef ::not_a_component_hacked_specialization_no_common_base component_base_type;
	};

	template<>
	struct component<::not_a_component_hacked_specialization_wrong_component_type_typedef>: detail::component_base {
		typedef component_a component_base_type;
	};
}

struct not_a_component_not_derived: slirc::detail::component_base {
	typedef not_a_component_not_derived component_base_type;
};
struct not_a_component_hacked_specialization_no_common_base: slirc::detail::component_base {
	typedef not_a_component_hacked_specialization_no_common_base component_base_type;
};
struct not_a_component_hacked_specialization_wrong_component_type_typedef
: slirc::component<not_a_component_hacked_specialization_wrong_component_type_typedef> {
};

TEST_CASE("eligibility of components - detail::is_valid_component", "") {
	SECTION("not valid: passing component<> directly") {
		// failing at 4 (see implementation of detail::is_valid_component)
		REQUIRE_FALSE(slirc::detail::is_valid_component<not_a_component_not_derived>());
	}

	SECTION("not valid: not derived from component<>") {
		// failing at 3 (see implementation of detail::is_valid_component)
		REQUIRE_FALSE(slirc::detail::is_valid_component<not_a_component_not_derived>());
	}

	SECTION("not valid: specialization hacked: base type typedef") {
		// failing at 2 (see implementation of detail::is_valid_component)
		REQUIRE_FALSE(slirc::detail::is_valid_component<not_a_component_hacked_specialization_wrong_component_type_typedef>());
	}

	SECTION("not valid: specialization hacked: not derived from common base") {
		// failing at 1 (see implementation of detail::is_valid_component)
		REQUIRE_FALSE(slirc::detail::is_valid_component<not_a_component_hacked_specialization_no_common_base>());
	}

	SECTION("valid: standalone component") {
		REQUIRE(slirc::detail::is_valid_component<component_a>());
	}

	SECTION("valid: base component in polymorphic component type") {
		REQUIRE(slirc::detail::is_valid_component<component_inherit_base>());
	}

	SECTION("valid: derived component in polymorphic component type") {
		REQUIRE(slirc::detail::is_valid_component<component_inherit_derived_a>());
	}
}

SCENARIO("component_container - basic insert, at, find, has, remove", "") {
	GIVEN("an empty component_container") {
		slirc::component_container cc;
		auto &cc_contents = slirc::test::test_overrides::component_containter_contents(cc);

		REQUIRE(cc_contents.empty());

		WHEN("inserting a component A") {
			REQUIRE_FALSE(cc.has<component_a>());
			REQUIRE_NOTHROW(cc.insert(component_a{}));

			THEN("the container contains one component") {
				REQUIRE(cc_contents.size() == 1);
			}

			THEN("we can get an component A from it") {
				REQUIRE(cc.has<component_a>());
				REQUIRE_NOTHROW(cc.at<component_a>());
				REQUIRE_FALSE(cc.find<component_a>() == nullptr);
			}

			THEN("we can not get an component B from it") {
				REQUIRE_FALSE(cc.has<component_b>());
				REQUIRE_THROWS_AS(cc.at<component_b>(), std::out_of_range);
				REQUIRE(cc.find<component_b>() == nullptr);
			}

			THEN("we can not insert another component A") {
				REQUIRE_THROWS_AS(cc.insert(component_a()), slirc::exceptions::component_conflict);
			}

			THEN("we can not remove a component B, and attempting to do so will not remove component A") {
				REQUIRE_FALSE(cc.remove<component_b>());
				REQUIRE(cc.has<component_a>());
			}
		}
	}

	GIVEN("component_container containing only a instance of component A") {
		slirc::component_container cc;
		auto &cc_contents = slirc::test::test_overrides::component_containter_contents(cc);

		REQUIRE_NOTHROW(cc.insert(component_a()));

		WHEN("removing the component A") {
			REQUIRE(cc.has<component_a>());
			REQUIRE(cc.remove<component_a>());

			THEN("the container is empty again") {
				REQUIRE_FALSE(cc.has<component_a>());
			}

			THEN("we can no longer get an component A from it") {
				REQUIRE_FALSE(cc.has<component_a>());
				REQUIRE_THROWS_AS(cc.at<component_a>(), std::out_of_range);
				REQUIRE(cc.find<component_a>() == nullptr);
			}

			THEN("we can no longer remove a component A") {
				REQUIRE_FALSE(cc.has<component_a>());
				REQUIRE_FALSE(cc.remove<component_a>());
				REQUIRE_FALSE(cc.has<component_a>());
			}

			THEN("we can insert a component A again") {
				REQUIRE_FALSE(cc.has<component_a>());
				REQUIRE_NOTHROW(cc.insert(component_a{}));
				REQUIRE(cc.has<component_a>());
			}
		}

		WHEN("inserting a component B") {
			REQUIRE_NOTHROW(cc.at<component_a>());
			REQUIRE(cc_contents.size() == 1);

			REQUIRE_NOTHROW(cc.insert<component_b>());

			THEN("the container contains two components") {
				REQUIRE(cc_contents.size() == 2);
			}

			THEN("the container contains a component A and a component B") {
				REQUIRE(cc.has<component_a>());
				REQUIRE(cc.has<component_b>());
				REQUIRE_NOTHROW(cc.at<component_a>());
				REQUIRE_NOTHROW(cc.at<component_b>());
			}

			THEN("neither another A nor another B can be inserted") {
				REQUIRE_THROWS_AS(cc.insert<component_a>(), slirc::exceptions::component_conflict);
				REQUIRE_THROWS_AS(cc.insert<component_b>(), slirc::exceptions::component_conflict);
			}
		}
	}
}

SCENARIO("component_container - handling inherited components", "") {
	GIVEN("an empty container") {
		slirc::component_container cc;

		WHEN("adding a component derived of a base component") {
			cc.insert(component_inherit_derived_a());

			THEN("the component can be accessed by its original type") {
				REQUIRE(cc.has<component_inherit_derived_a>());
				REQUIRE_NOTHROW(cc.at<component_inherit_derived_a>());
				REQUIRE_FALSE(cc.find<component_inherit_derived_a>() == nullptr);
			}

			THEN("the component can be accessed by its base component type") {
				REQUIRE(cc.has<component_inherit_base>());
				REQUIRE_NOTHROW(cc.at<component_inherit_base>());
				REQUIRE_FALSE(cc.find<component_inherit_base>() == nullptr);
			}

			THEN("the component can not be accessed by an unrelated component derived of the same base component") {
				REQUIRE_FALSE(cc.has<component_inherit_derived_b>());
				REQUIRE_THROWS_AS(cc.at<component_inherit_derived_b>(), slirc::exceptions::component_conflict);
				REQUIRE(cc.find<component_inherit_derived_b>() == nullptr);
			}

			THEN("the component can not be accessed by a more derived component type") {
				REQUIRE_FALSE(cc.has<component_inherit_derived_a_derived>());
				REQUIRE_THROWS_AS(cc.at<component_inherit_derived_a_derived>(), slirc::exceptions::component_conflict);
				REQUIRE(cc.find<component_inherit_derived_a_derived>() == nullptr);
			}

			THEN("no other component based on the same based component can be inserted") {
				REQUIRE_THROWS_AS(cc.insert(component_inherit_base()), slirc::exceptions::component_conflict);
				REQUIRE_THROWS_AS(cc.insert(component_inherit_derived_a()), slirc::exceptions::component_conflict);
				REQUIRE_THROWS_AS(cc.insert(component_inherit_derived_b()), slirc::exceptions::component_conflict);
				REQUIRE_THROWS_AS(cc.insert(component_inherit_derived_a_derived()), slirc::exceptions::component_conflict);
			}

			THEN("the component cannot be removed by an unrelated component derived of the same base component") {
				REQUIRE_THROWS_AS(cc.remove<component_inherit_derived_b>(), slirc::exceptions::component_conflict);
				REQUIRE(cc.has<component_inherit_derived_a>());
			}

			THEN("the component cannot be removed by a more derived component type") {
				REQUIRE_THROWS_AS(cc.remove<component_inherit_derived_a_derived>(), slirc::exceptions::component_conflict);
				REQUIRE(cc.has<component_inherit_derived_a>());
			}

			THEN("the component can be removed by its base component type") {
				REQUIRE(cc.remove<component_inherit_base>());
				REQUIRE_FALSE(cc.has<component_inherit_derived_a>());
			}
		}
	}
}

SCENARIO("component_container - at_or_insert<T,T>", "") {
	GIVEN("an empty container") {
		slirc::component_container cc;
		REQUIRE_FALSE(cc.has<component_inherit_derived_a>());

		WHEN("attempting to fetch or insert a component derived of a base component") {
			component_inherit_base *ptr;
			REQUIRE_NOTHROW(ptr = &cc.at_or_insert(component_inherit_derived_a()));

			THEN("the container contains the component") {
				REQUIRE(cc.has<component_inherit_derived_a>());
			}

			THEN("attempting to insert another component of the same type will yield the previous instance") {
				component_inherit_base *ptr2;
				REQUIRE_NOTHROW(cc.at<component_inherit_derived_a>());
				REQUIRE_NOTHROW(ptr2 = &cc.at_or_insert(component_inherit_derived_a()));
				REQUIRE(ptr == ptr2);
			}

			THEN("attempting to insert another component of the base component type will yield the previous instance") {
				component_inherit_base *ptr2;
				REQUIRE_NOTHROW(ptr2 = &cc.at_or_insert(component_inherit_base()));
				REQUIRE(ptr == ptr2);
			}

			THEN("attempting to insert an unrelated component derived of the same base component will fail") {
				REQUIRE_THROWS_AS(cc.at_or_insert(component_inherit_derived_b()), slirc::exceptions::component_conflict);
			}

			THEN("attempting to insert a more derived component type will fail") {
				REQUIRE_THROWS_AS(cc.at_or_insert(component_inherit_derived_a_derived()), slirc::exceptions::component_conflict);
			}
		}
	}
}

SCENARIO("component_container - at_or_insert<T,U>", "") {
	GIVEN("an empty container") {
		slirc::component_container cc;
		REQUIRE_FALSE(cc.has<component_inherit_derived_a>());

		WHEN("attempting to fetch the base component or insert a component derived of a base component") {
			component_inherit_base *ptr;
			REQUIRE_NOTHROW(ptr = &cc.at_or_insert<component_inherit_base>(component_inherit_derived_a()));

			THEN("the container contains the derived component") {
				REQUIRE(cc.has<component_inherit_derived_a>());
			}

			THEN("attempting to fetch the base component or insert another component of the same type will yield the previous instance") {
				component_inherit_base *ptr2;
				REQUIRE_NOTHROW(cc.at<component_inherit_derived_a>());
				REQUIRE_NOTHROW(ptr2 = &cc.at_or_insert<component_inherit_base>(component_inherit_derived_a()));
				REQUIRE(ptr == ptr2);
			}

			// skipping check for component_inherit_base, as that would invoke at_or_insert<T,T>

			THEN("attempting to fetch the base component or insert an unrelated component derived of the same base component will fail") {
				component_inherit_base *ptr2;
				REQUIRE_NOTHROW(ptr2 = &cc.at_or_insert<component_inherit_base>(component_inherit_derived_b()));
				REQUIRE(ptr == ptr2);
			}

			THEN("attempting to fetch the base component or insert a more derived component type will fail") {
				component_inherit_base *ptr2;
				REQUIRE_NOTHROW(ptr2 = &cc.at_or_insert<component_inherit_base>(component_inherit_derived_a_derived()));
				REQUIRE(ptr == ptr2);
			}

			REQUIRE(cc.remove<component_inherit_base>());
		}

		WHEN("inserting the base component of a derived component type") {
			REQUIRE_NOTHROW(cc.insert(component_inherit_base()));

			THEN("attempting to fetch a derived component or insert an even more derived component will fail") {
				REQUIRE_THROWS_AS(cc.at_or_insert<component_inherit_derived_a>(component_inherit_derived_a_derived()), slirc::exceptions::component_conflict);
			}
		}
	}
}
