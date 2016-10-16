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

#include "../include/slirc/network.hpp"

#include <mutex>
#include <stdexcept>
#include <thread>

#if __has_include(<optional>)
#	include <optional>
using std::optional;
using std::nullopt;
#elif __has_include(<experimental/optional>)
#	include <experimental/optional>
using std::experimental::optional;
using std::experimental::nullopt;
#else
#	error Neither std::optional nor std::experimental::optional are supported
#endif

#include <boost/asio/io_service.hpp>

namespace {
	struct slirc_network_service {
		slirc_network_service()
		: service_mutex()
		, service_thread()
		, internal_service()
		, current_service(&internal_service)
		, internal_service_work()
		, internal_service_stopping(false) {}

		slirc_network_service(const slirc_network_service &) = delete;
		slirc_network_service& operator=(const slirc_network_service &) = delete;

		~slirc_network_service() {
			{ std::unique_lock<std::mutex> lock(service_mutex);
				internal_service_stopping = true;
				if (service_thread.joinable()) {
					internal_service_work = nullopt;
					lock.unlock();
					service_thread.join();
				}
			}
		}

		boost::asio::io_service &get_service() {
			// assume: service_mutex locked!
			if (current_service != &internal_service) {
				return *current_service;
			}
			else if (internal_service_stopping) {
				throw std::logic_error("slirc::network: must not request internal service during service shutdown!");
			}
			else {
				if (!service_thread.joinable()) {
					// internal service not started yet;
					//   attach work object and run in thread!
					internal_service_work.emplace(internal_service);
					service_thread = std::thread([&]{ internal_service.run(); });
				}
				return internal_service;
			}
		}

		std::mutex service_mutex;
			std::thread service_thread;
			boost::asio::io_service internal_service;
			boost::asio::io_service *current_service;
			optional<boost::asio::io_service::work> internal_service_work;
			bool internal_service_stopping;
	} network_service;
}

boost::asio::io_service &slirc::network::service() {
	std::unique_lock<std::mutex> lock(network_service.service_mutex);
	return network_service.get_service();
}

void slirc::network::service(boost::asio::io_service &external_service) {
	std::unique_lock<std::mutex> lock(network_service.service_mutex);
	network_service.current_service = &external_service;
}

bool slirc::network::uses_internal_service() {
	std::unique_lock<std::mutex> lock(network_service.service_mutex);
	return network_service.current_service == &network_service.internal_service;
}

bool slirc::network::has_ssl_support() {
#ifdef SLIRC_BUILD_NO_SSL
	return false;
#else
	return true;
#endif
}
