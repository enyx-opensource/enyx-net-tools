#define BOOST_TEST_MODULE Iperf

#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/process.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

namespace p = boost::process;
namespace ip = boost::asio::ip;

struct TcpFixture
{
    TcpFixture()
        : io_service_(),
          peer_socket_(io_service_),
          peer_acceptor_(io_service_),
          iperf_server_stdout_(io_service_),
          iperf_(),
          endpoint_(ip::address::from_string("127.0.0.1"),
                    1234),
          iperf_buffer_(),
          peer_buffer_(1024 * 1024)
    { }

    void
    start_iperf_server(const std::string & args)
    {
        std::ostringstream iperf_server_cmd;
        iperf_server_cmd << IPERF_BINARY_PATH
                         << " --listen=" << endpoint_.address()
                                         << ":"
                                         << endpoint_.port()
                         << " " << args;

        iperf_ = p::child{iperf_server_cmd.str(),
                          p::std_in < p::null,
                          p::std_out > iperf_server_stdout_,
                          io_service_};

        BOOST_TEST_CHECKPOINT("iperf client started");

        async_read_iperf_output();
    }

    void
    start_iperf_client(const std::string & args)
    {
        std::ostringstream iperf_client_cmd;
        iperf_client_cmd << IPERF_BINARY_PATH
                         << " --connect=" << endpoint_.address()
                                         << ":"
                                         << endpoint_.port()
                         << " " << args;

        iperf_ = p::child{iperf_client_cmd.str(),
                          p::std_in < p::null,
                          p::std_out > p::null,
                          io_service_};

        BOOST_TEST_CHECKPOINT("iperf client started");
    }

    void
    wait_for_iperf()
    {
        iperf_.wait();
        BOOST_REQUIRE_EQUAL(0, iperf_.exit_code());
    }

    void
    async_read_iperf_output()
    {
        boost::asio::async_read_until(iperf_server_stdout_,
                                      iperf_buffer_,
                                      '\n',
                                      boost::bind(&TcpFixture::on_iperf_receive, this, _1, _2));
    }

    void
    start_peer_server()
    {
        peer_acceptor_.open(endpoint_.protocol());
        ip::tcp::acceptor::reuse_address reuse_address{true};
        peer_acceptor_.set_option(reuse_address);
        peer_acceptor_.bind(endpoint_);
        peer_acceptor_.listen();
        peer_acceptor_.async_accept(peer_socket_,
                                    boost::bind(&TcpFixture::on_peer_connect, this, _1));
    }

    void
    on_iperf_receive(const boost::system::error_code & failure,
                     std::size_t bytes_received)
    {
        if (failure)
            BOOST_REQUIRE_EQUAL(boost::asio::error::eof, failure);
        else
        {
            std::string line;
            {
                std::istream is(&iperf_buffer_);
                std::getline(is, line);
            }
            if (line.find("Waiting") == 0 && ! peer_socket_.is_open())
                peer_socket_.async_connect(endpoint_,
                                           boost::bind(&TcpFixture::on_peer_connect, this, _1));

            async_read_iperf_output();
        }
    }

    void
    on_peer_connect(const boost::system::error_code & failure)
    {
        BOOST_REQUIRE(! failure);
        async_read_peer();
    }

    void
    async_read_peer()
    {
        boost::asio::async_read(peer_socket_,
                                boost::asio::buffer(peer_buffer_),
                                boost::asio::transfer_all(),
                                boost::bind(&TcpFixture::on_peer_receive, this, _1, _2));
    }

    void
    on_peer_receive(const boost::system::error_code & failure,
                    std::size_t bytes_received)
    {
        if (failure)
        {
            BOOST_REQUIRE_EQUAL(boost::asio::error::eof, failure);
            peer_socket_.close();
        }
        else
            boost::asio::async_write(peer_socket_,
                                     boost::asio::buffer(peer_buffer_, bytes_received),
                                     boost::asio::transfer_all(),
                                     boost::bind(&TcpFixture::on_peer_sent, this, _1, _2));
    }

    void
    on_peer_sent(const boost::system::error_code & failure,
                 std::size_t bytes_received)
    {
        BOOST_REQUIRE(! failure);

        async_read_peer();
    }

    boost::asio::io_service io_service_;
    ip::tcp::socket peer_socket_;
    ip::tcp::acceptor peer_acceptor_;
    p::async_pipe iperf_server_stdout_;
    p::child iperf_;
    ip::tcp::endpoint endpoint_;
    boost::asio::streambuf iperf_buffer_;
    std::vector<std::uint8_t> peer_buffer_;
};

BOOST_FIXTURE_TEST_SUITE(ServerTcp, TcpFixture)

BOOST_AUTO_TEST_CASE(VerifyNone)
{
    start_iperf_server("--size=1MiB --verify=none");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(VerifyFirst)
{
    start_iperf_server("--size=1MiB --verify=first");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(VerifyAll)
{
    start_iperf_server("--size=1MiB --verify=all");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(ClientTcp, TcpFixture)

BOOST_AUTO_TEST_CASE(VerifyNone)
{
    start_peer_server();
    start_iperf_client("--size=1MiB --verify=none");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(VerifyFirst)
{
    start_peer_server();
    start_iperf_client("--size=1MiB --verify=first");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(VerifyAll)
{
    start_peer_server();
    start_iperf_client("--size=1MiB --verify=all");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_SUITE_END()

