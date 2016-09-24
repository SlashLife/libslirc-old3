#include <cassert>

#include <slirc/event.hpp>

enum class my_events: slirc::event::underlying_id_type {
	startup,
	shutdown
};
SLIRC_REGISTER_EVENT_ID_ENUM(my_events);

static_assert(slirc::event::is_valid_id_type<my_events>(), "Must be valid!");

int main() {
	slirc::event::id_type id = my_events::startup;
}
