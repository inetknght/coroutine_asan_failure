#include <cstdlib>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#define STRINGIFY(x) #x
#define XSTRINGIFY(x) STRINGIFY(x)
#ifdef BOOST_USE_ASAN
constexpr std::string_view boost_use_asan{XSTRINGIFY(BOOST_USE_ASAN)};
#else
constexpr std::string_view boost_use_asan{"NOT DEFINED"};
#endif

#include <boost/asio/write.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ip/tcp.hpp>
namespace asio = boost::asio;

#include <spdlog/spdlog.h>
#include <spdlog/formatter.h>

template <>
struct fmt::formatter<asio::ip::tcp::endpoint>
: public formatter<std::string>
{
	format_context::iterator
	format(
		const asio::ip::tcp::endpoint& e,
		format_context& ctx
	) const;
};

fmt::format_context::iterator
fmt::formatter<asio::ip::tcp::endpoint>::format(
	const asio::ip::tcp::endpoint& e,
	format_context& ctx
) const
{
	std::stringstream s;
	s << e;
	return formatter<std::string>::format(s.str(), ctx);
}

asio::ip::tcp::endpoint
parse_address(
	std::string_view address,
	std::uint16_t default_port
)
{
	asio::ip::tcp::endpoint ep;
	auto colonPos = address.find(':');
	if (colonPos != std::string::npos)
	{
		ep.port(
			std::stoi(
				std::string{
					address.substr(1+colonPos)
				}
			)
		);
	}
	else
	{
		colonPos = address.size();
		ep.port(default_port);
	}
	ep.address(
		asio::ip::address::from_string(
			std::string{
				address.substr(0, colonPos)
			}
		)
	);
	return ep;
}

constexpr auto reply = std::string_view{
	"HTTP/1.1 204 No Content\r\n"
	"Server: 127.0.0.1:8080\r\n"
	"Connection: close\r\n"
	"\r\n"
};

asio::awaitable<void>
async_listen(
	asio::io_context& iox)
{
	auto ep = parse_address({"127.0.0.1"}, 8080);

	asio::ip::tcp::acceptor acceptor{iox};
	acceptor.open(ep.protocol());
	acceptor.set_option(asio::socket_base::reuse_address{true});
	acceptor.bind(ep);
	acceptor.listen();

	ep = acceptor.local_endpoint();
	spdlog::info("listening on {}", ep);
	while(true)
	{
		auto s= co_await acceptor.async_accept(asio::use_awaitable);
		spdlog::info("connection from {}", s.remote_endpoint());
		asio::spawn(
			acceptor.get_executor(),
			[&, socket = std::move(s)](
				asio::yield_context yield
			) mutable
			{
				boost::system::error_code ec;
				auto written = asio::async_write(
					socket,
					asio::buffer(reply),
					yield[ec]
				);
				throw std::runtime_error{std::to_string(written)};
			},
			[](std::exception_ptr e)
			{
				if (e)
				{
					std::rethrow_exception(e);
				}
			}
		);
	}
}

int main()
{
	spdlog::info("BOOST_USE_ASAN: {}", boost_use_asan);

	asio::io_context iox;

	co_spawn(
		iox,
		async_listen(iox),
		[](std::exception_ptr e)
		{
			if (e)
			{
				std::rethrow_exception(e);
			}
		}
	);

	try
	{
		iox.run();
	}
	catch (const std::runtime_error& e)
	{
		if (std::string_view{e.what()} != std::to_string(reply.size()))
		{
			throw;
		}
	}
	return EXIT_SUCCESS;
}
