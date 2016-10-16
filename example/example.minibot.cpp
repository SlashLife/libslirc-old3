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

#include <iostream>

#include <slirc/irc.hpp>
#include <slirc/modules/connection.hpp>

int main(int argc, char **argv) {
	((void)argc);
	((void)argv);

	slirc::irc irc;
	auto &connection = irc.load<slirc::modules::connection>();

	connection.connect("irc://irc.freenode.org:6667");

	irc.event_manager().connect(
		slirc::modules::connection::state::connected,
		[&](slirc::event::pointer){
			connection.send_raw("NICK slircbot_\r\n");
			connection.send_raw("USER slircbot_ * * :libslIRC bot\r\n");
		}
	);

	irc.event_manager().connect(
		slirc::modules::connection::received_line,
		[&](slirc::event::pointer e){
			std::string &data = e->components.at<slirc::apis::connection::received_data>().data;
			std::cout << data << "\n";

			if (std::string::npos != data.find(" 001 ")) {
				connection.send_raw("JOIN #php.bottest\r\n");
			}

			if (data.substr(0,5) == "PING ") {
				std::string pong = data + "\r\n";
				pong[1] = 'O';
				connection.send_raw(pong.c_str(), pong.size());
				pong = "PRIVMSG #php.bottest :"+pong;
				connection.send_raw(pong.c_str(), pong.size());
			}
		}
	);

	slirc::event::pointer e;
	do {
		e = irc.event_manager().wait_event();
		if (e) {
			e->handle();
		}
	} while(e && e->original_id != slirc::apis::connection::state::disconnected);

	std::cout << "disconnected\n";
}
