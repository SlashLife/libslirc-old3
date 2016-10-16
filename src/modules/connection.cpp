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

#include "../../include/slirc/modules/connection.hpp"

#include <cstdlib>

#include <algorithm>
#include <array>
#include <mutex>
#include <vector>

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

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#ifndef SLIRC_BUILD_NO_SSL
#	include <boost/asio/ssl/context.hpp>
#	include <boost/asio/ssl/stream.hpp>
#endif

#include "../../include/slirc/exceptions.hpp"
#include "../../include/slirc/irc.hpp"
#include "../../include/slirc/network.hpp"

#undef IF_SSL
#ifdef SLIRC_BUILD_NO_SSL
#	define IF_SSL(ssl, content) if (false) {}
#else
#	define IF_SSL(ssl, content) if (ssl) { content }
#endif

namespace asio = boost::asio;
using tcp_socket =
	asio::basic_socket<
		asio::ip::tcp,
		asio::stream_socket_service<
			asio::ip::tcp
		>
	>;
using tcp_resolver = asio::ip::tcp::resolver;



struct slirc::modules::connection::error_info::impl {
	impl(std::string &&msg_, const boost::system::error_code &ec_)
	: msg(std::move(msg_))
	, ec(ec_) {}

	const std::string msg;
	const boost::system::error_code ec;
};

const std::string &slirc::modules::connection::error_info::message() const {
	SLIRC_ASSERT( impl_ && "error_info was not correctly initialized!" );
	return impl_->msg;
}

const boost::system::error_code &slirc::modules::connection::error_info::error_code() const {
	SLIRC_ASSERT( impl_ && "error_info was not correctly initialized!" );
	return impl_->ec;
}



struct slirc::modules::connection::impl: std::enable_shared_from_this<impl> {
	typedef std::weak_ptr<slirc::modules::connection::impl> weak_impl;
	typedef std::shared_ptr<slirc::modules::connection::impl> locked_impl;

#ifndef SLIRC_BUILD_NO_SSL
	struct ssl_impl {
		ssl_impl(asio::io_service &service)
		: context(asio::ssl::context::sslv23_client)
		, socket(service, context) {}

		asio::ssl::context context;
		asio::ssl::stream<asio::ip::tcp::socket> socket;
	};
#endif

	slirc::modules::connection &module;
	std::mutex mutex;
		std::string hostname;
		unsigned port;
		state curstate;
		optional<tcp_resolver> resolver;
		tcp_resolver::iterator resolve_iterator;
		optional<asio::ip::tcp::socket> socket;
#ifndef SLIRC_BUILD_NO_SSL
		optional<ssl_impl> ssl;
#endif
		struct buffers_ {
			typedef std::vector<char> buffer;
			typedef std::array<char, 640> raw_buffer;

			buffers_(impl &impl_)
			: imp(impl_)
			, send_buffer()
			, fill_buffer()
			, recv_buffer()
			, raw_recv_buffer(std::make_shared<raw_buffer>())
			, current_send_state(send_buffer.end())
			, send_job_running(false) {}

			void append(const char *data, std::size_t length) {
				// assumes mutex to be locked, and the connection to be established!
				buffer::size_type prev_size = fill_buffer.size();
				fill_buffer.resize(prev_size + length);
				std::copy(data, data+length, fill_buffer.begin()+prev_size);

				if (!send_job_running && 0 < length) {
					send_job_running = true;
					send();
				}
			}

			void clear() {
				send_buffer.clear();
				fill_buffer.clear();
				recv_buffer.clear();
				send_job_running = false;
			}

			void swap() {
				// assumes mutex to be locked!
				std::swap(send_buffer, fill_buffer);
				current_send_state = send_buffer.begin();
				fill_buffer.clear();
			}

			void send() {
				// assumes mutex to be locked, and the connection to be established!
				if (send_buffer.end() == current_send_state) {
					swap();
				}
				if (send_buffer.end() == current_send_state) {
					send_job_running = false;
					return;
				}

				const auto &send_callback =
					[&, self = weak_impl(imp.shared_from_this())](
						const boost::system::error_code& error,
						std::size_t num_transferred
					) {
						locked_impl impl_ = self.lock();
						if (!impl_) return; // implementation has been destroyed

						std::unique_lock<std::mutex> lock(imp.mutex);
						if (imp.curstate != state::connected) {
							return; // probably aborted
						}

						if (error) {
							imp.emit_error("Sending data failed: " + error.message(), error);
							imp.do_unscheduled_disconnect();
						}
						else {
							current_send_state += num_transferred;
							send();
						}
					};


				IF_SSL(imp.ssl,
					imp.ssl->socket.async_write_some(
						asio::const_buffers_1(&*current_send_state, send_buffer.end()-current_send_state),
						send_callback
					);
				)
				else {
					imp.socket->async_write_some(
						asio::const_buffers_1(&*current_send_state, send_buffer.end()-current_send_state),
						send_callback
					);
				}
			}

			impl &imp;
			buffer send_buffer;
			buffer fill_buffer;
			std::string recv_buffer;
			std::shared_ptr<raw_buffer> raw_recv_buffer;
			buffer::iterator current_send_state;
			bool send_job_running;
		} buffers;

	static constexpr unsigned default_port_nonssl = 6667;
	static constexpr unsigned default_port_ssl    = 6697;

	impl(slirc::modules::connection &module)
	: module(module)
	, mutex()
	, hostname("0.0.0.0")
	, port(default_port_nonssl)
	, curstate(state::disconnected)
	, resolver()
	, resolve_iterator()
	, socket()
#ifndef SLIRC_BUILD_NO_SSL
	, ssl()
#endif
	, buffers(*this) {}

	void set_endpoint(const std::string &new_endpoint, unsigned new_port) {
		std::unique_lock<std::mutex> lock(mutex);
		if (curstate != state::disconnected) {
			throw exceptions::already_connected();
		}

		bool use_ssl = false;

		std::string::size_type
			server_port_begin = 0,
			server_port_end = new_endpoint.size();

		std::string::size_type pos = new_endpoint.find("://", server_port_begin);
		if (pos != std::string::npos) {
			// using URL format
			{ const std::string proto = new_endpoint.substr(0, pos);
				if (proto == "irc" || proto == "tcp") {
					use_ssl = false; // again ...
				}
				else if (proto == "ircs" || proto == "ssl") {
					use_ssl = true;
				}
				else {
					return set_invalid();
				}
			}

			// chop off protocol and "://" at the beginning
			server_port_begin = pos+3;

			// chop off path/query/fragment parts at the end
			pos = new_endpoint.find_first_of("/?#", server_port_begin);
			if (pos != std::string::npos) {
				server_port_end = pos;
			}

			// chop off authentication info at the beginning
			pos = new_endpoint.rfind('@', server_port_end);
			if (pos != std::string::npos && server_port_begin <= pos) {
				server_port_begin = pos+1;
			}
		}

		// set default port
		unsigned use_port = (use_ssl ? default_port_ssl : default_port_nonssl);

		// override with port given in endpoint
		pos = new_endpoint.find_last_not_of("0123456789", server_port_end);
		if (pos != std::string::npos && server_port_begin <= pos && new_endpoint[pos]==':') {
			// found port number!
			char *port_end=nullptr;
			((void)port_end); // used only for assertion

			use_port = strtol(new_endpoint.c_str() + pos + 1, &port_end, 10);
			SLIRC_ASSERT( port_end == new_endpoint.c_str() + server_port_end
				&& "end of port definition does not line up with end of server/port pair" );

			server_port_end = pos;
		}

		// override with explicitly passed port number
		if (new_port != 0) {
			use_port = new_port;
		}

		if (server_port_end <= server_port_begin) {
			return set_invalid();
		}

		hostname = new_endpoint.substr(server_port_begin, server_port_end-server_port_begin);
		port = use_port;

		// set up either ssl or non-ssl, and tear down the other
		if (use_ssl) {
#ifndef SLIRC_BUILD_NO_SSL
			ssl.emplace(network::service());
			socket = nullopt;
#else
			throw std::logic_error("Attempting to use SSL in libslirc, but libslirc was built without SSL support.");
#endif
		}
		else {
#ifndef SLIRC_BUILD_NO_SSL
			ssl = nullopt;
#endif
			socket.emplace(network::service());
		}
	}

	void connect() {
		std::unique_lock<std::mutex> lock(mutex);
		if (curstate != state::disconnected) {
			throw exceptions::already_connected();
		}

		// Connecting happens in the following stages:
		// - emit state::connecting (here)
		// - host name look up (connect_resolve)
		// - connecting to the looked up endpoint (connect_try_next)
		// - SSL handshake, if required (connect_opt_ssl_handshake)
		// - emit state::connected (connect_success)

		emit_state_change(state::connecting);
		connect_resolve();
	}

	void disconnect() {
		std::unique_lock<std::mutex> lock(mutex);
		if (curstate == state::disconnecting || curstate == state::disconnected) {
			return; // already disconnected or disconnecting
		}

		do_disconnect();
	}

	state current_state() {
		std::unique_lock<std::mutex> lock(mutex);
		return curstate;
	}

private:
	void connect_resolve() {
		// assumes mutex to be locked!
		resolver.emplace(network::service());
		resolver->async_resolve(
			tcp_resolver::query(
				hostname,
				std::to_string(port)
			),
			[&, self=weak_impl(shared_from_this())](
				const boost::system::error_code &error,
				tcp_resolver::iterator iterator
			) {
				locked_impl impl_ = self.lock();
				if (!impl_) return; // implementation has been destroyed

				std::unique_lock<std::mutex> lock(mutex);
				if (curstate != state::connecting) {
					return; // probably aborted
				}

				if (error) {
					emit_error("Name lookup failed: " + error.message(), error);
					do_unscheduled_disconnect();
				}
				else {
					resolve_iterator = iterator;
					connect_try_next();
				}
			}
		);
	}

	void connect_try_next() {
		// assumes mutex to be locked!
		if (resolve_iterator == decltype(resolve_iterator){}) {
			do_unscheduled_disconnect();
		}
		else {
			auto &sock = get_socket();
			sock.async_connect(
				*resolve_iterator,
				[&, self=weak_impl(shared_from_this())](
					const boost::system::error_code &error
				) {
					locked_impl impl_ = self.lock();
					if (!impl_) return; // implementation has been destroyed

					std::unique_lock<std::mutex> lock(mutex);
					if (curstate != state::connecting) {
						return; // probably aborted
					}

					if (error) {
						emit_error("Connection failed: " + error.message(), error);
						connect_try_next();
					}
					else {
						connect_opt_ssl_handshake();
					}
				}
			);
			++resolve_iterator;
		}
	}

	void connect_opt_ssl_handshake() {
		// assumes mutex to be locked!
		IF_SSL(ssl,
			ssl->socket.async_handshake(
				asio::ssl::stream<tcp_socket>::client,
				[&, self=weak_impl(shared_from_this())](
					const boost::system::error_code &error
				) {
					locked_impl impl_ = self.lock();
					if (!impl_) return; // implementation has been destroyed

					std::unique_lock<std::mutex> lock(mutex);
					if (curstate != state::connecting) {
						return; // probably aborted
					}

					if (error) {
						emit_error("SSL handshake failed: " + error.message(), error);
					}
					else {
						connect_success();
					}
				}
			);
		)
		else {
			connect_success();
		}
	}

	void connect_success() {
		// assumes mutex to be locked!
		clear_resolver(); // no longer needed
		buffers.clear();
		emit_state_change(state::connected);
		recv();
	}



	void clear_resolver() {
		if (resolver) {
			resolver->cancel();
			resolver = nullopt;
			resolve_iterator = decltype(resolve_iterator){};
		}
	}

	void do_disconnect() {
		boost::system::error_code ec;
		// assumes mutex to be locked, and state to be connecting or connected!
		emit_state_change(state::disconnecting);
		do_unscheduled_disconnect();
	}

	void do_unscheduled_disconnect() {
		// assumes mutex to be locked, and state to be connecting or connected!
		boost::system::error_code ec;

		auto &sock = get_socket();
		sock.shutdown(tcp_socket::shutdown_both, ec); // ignore error code
		sock.cancel();
		sock.close(ec); // ignore error code

		clear_resolver();
		buffers.clear();

		emit_state_change(state::disconnected);
	}

	void emit_error(std::string &&error_message, const boost::system::error_code &error_code) {
		event::pointer e = module.irc.make_event(events::error);
		e->components.insert(error_info())
			.impl_ = std::make_unique<error_info::impl>(std::move(error_message), error_code);
		e->queue();
	}

	void emit_line(std::string &&line) {
		event::pointer e = module.irc.make_event(received_line);
		e->components.insert(received_data()).data = std::move(line);
		e->queue();
	}

	void emit_state_change(state newstate) {
		// assumes mutex to be locked!
		if (curstate != newstate) {
			event::pointer e = module.irc.make_event(newstate);
			e->queue_as(state::changed, event::queuing_position::at_front);

			curstate = newstate;
			e->queue();
		}
	}

	tcp_socket &get_socket() {
		// assumes mutex to be locked!
#ifndef SLIRC_BUILD_NO_SSL
		SLIRC_ASSERT( (socket || ssl)
			&& "neither non-ssl nor ssl is initialized!" );
#else
		SLIRC_ASSERT( (socket)
			&& "socket is not initialized!" );
#endif

		if (socket) {
			return *socket;
		}
		else IF_SSL(ssl,
			return ssl->socket.lowest_layer();
		)
		else {
			throw std::logic_error(
				"slirc::module::connection - socket is requested, "
				"but neither ssl nor non-ssl is initialized");
		}
	}

	void recv() {
		// assumes mutex to be locked!
		SLIRC_ASSERT( curstate == state::connected
			&& "must be connected to receive!" );

		std::string::size_type line_begin=0, line_end=0;

		while(true) {
			auto &buf = buffers.recv_buffer;

			line_begin = buf.find_first_not_of(" \r\n", line_end);
			if (line_begin == std::string::npos) {
				buf.clear();
				break;
			}

			line_end = buffers.recv_buffer.find("\n", line_begin);
			if (line_end == std::string::npos) {
				buf.erase(0, line_begin);
				break;
			}

			if (buf[line_end-1] == '\r') {
				--line_end;
			}

			emit_line(buf.substr(line_begin, line_end-line_begin));
		}

		const auto recv_handler =
			[&, self=weak_impl(shared_from_this()), buf=buffers.raw_recv_buffer](
				const boost::system::error_code& error,
				std::size_t bytes_transferred
			){
				locked_impl impl_ = self.lock();
				if (!impl_) return; // implementation has been destroyed

				std::unique_lock<std::mutex> lock(mutex);
				if (curstate != state::connected) {
					return; // probably aborted
				}

				if (error) {
					emit_error("Connection failed: " + error.message(), error);
					do_unscheduled_disconnect();
				}
				else {
					buffers.recv_buffer.append(
						&*buffers.raw_recv_buffer->begin(),
						bytes_transferred
					);
					recv();
				}
			};

		IF_SSL(ssl,
			ssl->socket.async_read_some(
				asio::mutable_buffers_1(
					&*buffers.raw_recv_buffer->begin(),
					buffers.raw_recv_buffer->size()
				),
				recv_handler
			);
		)
		else {
			socket->async_read_some(
				asio::mutable_buffers_1(
					&*buffers.raw_recv_buffer->begin(),
					buffers.raw_recv_buffer->size()
				),
				recv_handler
			);
		}
	}

	void set_invalid() {
		// assumes mutex to be locked!
		SLIRC_ASSERT( curstate == state::disconnected
			&& "must not change endpoint while connected or connecting");
		hostname = "0.0.0.0";
		port = 0;
#ifndef SLIRC_BUILD_NO_SSL
		ssl = nullopt;
#endif
	}
};

slirc::modules::connection::connection(slirc::irc &irc)
: apis::connection(irc)
, impl_(std::make_shared<impl>(*this)) {}

slirc::modules::connection::~connection() {
	impl_->disconnect();
}

void slirc::modules::connection::set_endpoint(const std::string &endpoint, unsigned port) {
	impl_->set_endpoint(endpoint, port);
}

void slirc::modules::connection::connect() {
	impl_->connect();
}

void slirc::modules::connection::disconnect() {
	impl_->disconnect();
}

slirc::modules::connection::state slirc::modules::connection::current_state() {
	return impl_->current_state();
}

void slirc::modules::connection::do_send_raw(const char *data, std::size_t length) {
	impl_->buffers.append(data, length);
}
