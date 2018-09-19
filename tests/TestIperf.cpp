#define BOOST_TEST_MODULE Iperf

#include <cstdint>
#include <vector>
#include <iostream>

#include <boost/test/unit_test.hpp>
#include <boost/process.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

namespace p = boost::process;
namespace ip = boost::asio::ip;

static constexpr std::size_t PAYLOAD_SIZE = 1024 * 1024;

struct TcpFixture
{
    enum Direction { TO_NXIPERF, FROM_NXIPERF, BOTH };

    TcpFixture()
        : io_service_(),
          peer_socket_(io_service_),
          peer_acceptor_(io_service_),
          iperf_server_stdout_(io_service_),
          iperf_(),
          endpoint_(ip::address::from_string("127.0.0.1"),
                    1234),
          iperf_buffer_(),
          peer_buffer_(1024 * 1024),
          requested_size_(),
          direction_()
    { }

    void
    start_iperf_server(std::size_t requested_size,
                       const std::string & args = std::string{},
                       Direction direction = BOTH)
    {
        requested_size_ = requested_size;
        direction_ = direction;

        std::ostringstream iperf_server_cmd;
        iperf_server_cmd << IPERF_BINARY_PATH
                         << " --listen=" << endpoint_.address()
                                         << ":"
                                         << endpoint_.port()
                         << " --size=" << requested_size_ << "B"
                         << " " << args;

        iperf_ = p::child{iperf_server_cmd.str(),
                          p::std_in < p::null,
                          p::std_out > iperf_server_stdout_,
                          p::std_err > stderr,
                          io_service_};

        BOOST_TEST_CHECKPOINT("iperf client started");

        async_read_iperf_output();
    }

    void
    start_iperf_client(std::size_t requested_size,
                       const std::string & args = std::string{},
                       Direction direction = BOTH)
    {
        requested_size_ = requested_size;
        direction_ = direction;

        std::ostringstream iperf_client_cmd;
        iperf_client_cmd << IPERF_BINARY_PATH
                         << " --connect=127.0.0.1:0:"
                         << endpoint_.address() << ":" << endpoint_.port()
                         << " --size=" << requested_size_ << "B"
                         << " " << args;

        iperf_ = p::child{iperf_client_cmd.str(),
                          p::std_in < p::null,
                          p::std_out > iperf_server_stdout_,
                          p::std_err > stderr,
                          io_service_};

        BOOST_TEST_CHECKPOINT("iperf client started");

        async_read_iperf_output();
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

        std::cout << "tester: Waiting for iperf to connect" << std::endl;
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
                                      boost::bind(&TcpFixture::on_iperf_output_receive, this, _1, _2));
    }

    void
    on_iperf_output_receive(const boost::system::error_code & failure,
                            std::size_t bytes_received)
    {
        if (! failure)
        {
            std::string line;
            {
                std::istream is(&iperf_buffer_);
                std::getline(is, line);
            }

            if (! line.empty())
                std::cout << "nx-iperf: " << line << std::endl;

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
        if (direction_ == FROM_NXIPERF)
        {
            peer_socket_.shutdown(peer_socket_.shutdown_send);
            async_read_rx_peer();
        }
        else if (direction_ == TO_NXIPERF)
            async_write_tx_peer();
        else
            async_read_rxtx_peer();

        std::cout << "tester: Connected to nx-iperf" << std::endl;
    }

    void
    async_read_rxtx_peer()
    {
        boost::asio::async_read(peer_socket_,
                                boost::asio::buffer(peer_buffer_),
                                boost::asio::transfer_all(),
                                boost::bind(&TcpFixture::on_peer_rxtx_receive, this, _1, _2));
    }

    void
    on_peer_rxtx_receive(const boost::system::error_code & failure,
                         std::size_t bytes_received)
    {
        if (failure)
        {
            BOOST_REQUIRE_EQUAL(boost::asio::error::eof, failure);
            peer_socket_.shutdown(peer_socket_.shutdown_send);
            peer_socket_.close();
        }
        else
            boost::asio::async_write(peer_socket_,
                                     boost::asio::buffer(peer_buffer_, bytes_received),
                                     boost::asio::transfer_all(),
                                     boost::bind(&TcpFixture::on_peer_rxtx_sent, this, _1, _2));
    }

    void
    on_peer_rxtx_sent(const boost::system::error_code & failure,
                      std::size_t bytes_sent)
    {
        BOOST_REQUIRE(! failure);

        BOOST_REQUIRE_LE(bytes_sent, requested_size_);
        requested_size_ -= bytes_sent;

        async_read_rxtx_peer();
    }

    void
    async_read_rx_peer()
    {
        boost::asio::async_read(peer_socket_,
                                boost::asio::buffer(peer_buffer_),
                                boost::asio::transfer_all(),
                                boost::bind(&TcpFixture::on_peer_rx_receive, this, _1, _2));
    }

    void
    on_peer_rx_receive(const boost::system::error_code & failure,
                       std::size_t bytes_received)
    {
        if (failure)
        {
            BOOST_REQUIRE_EQUAL(boost::asio::error::eof, failure);
            peer_socket_.close();
        }
        else
        {
            BOOST_REQUIRE_LE(bytes_received, requested_size_);
            requested_size_ -= bytes_received;

            async_read_rx_peer();
        }
    }

    void
    async_write_tx_peer()
    {
        const std::size_t size = std::min(peer_buffer_.size(), requested_size_);

        boost::asio::async_write(peer_socket_,
                                 boost::asio::buffer(peer_buffer_, size),
                                 boost::asio::transfer_all(),
                                 boost::bind(&TcpFixture::on_peer_tx_sent, this, _1, _2));
    }

    void
    on_peer_tx_sent(const boost::system::error_code & failure,
                    std::size_t bytes_sent)
    {
        BOOST_REQUIRE(! failure);

        BOOST_REQUIRE_LE(bytes_sent, requested_size_);
        requested_size_ -= bytes_sent;

        if (requested_size_)
            async_write_tx_peer();
        else
        {
            peer_socket_.shutdown(peer_socket_.shutdown_send);
            boost::asio::async_read(peer_socket_,
                                    boost::asio::buffer(peer_buffer_),
                                    boost::asio::transfer_all(),
                                    boost::bind(&TcpFixture::on_peer_tx_receive, this, _1, _2));
        }
    }

    void
    on_peer_tx_receive(const boost::system::error_code & failure,
                       std::size_t bytes_sent)
    {
        BOOST_REQUIRE_EQUAL(boost::asio::error::eof, failure);
        peer_socket_.close();
    }

    boost::asio::io_service io_service_;
    ip::tcp::socket peer_socket_;
    ip::tcp::acceptor peer_acceptor_;
    p::async_pipe iperf_server_stdout_;
    p::child iperf_;
    ip::tcp::endpoint endpoint_;
    boost::asio::streambuf iperf_buffer_;
    std::vector<std::uint8_t> peer_buffer_;
    std::size_t requested_size_;
    Direction direction_;
};

BOOST_FIXTURE_TEST_SUITE(ServerTcp, TcpFixture)

BOOST_AUTO_TEST_CASE(ShutdownSendComplete)
{
    start_iperf_server(PAYLOAD_SIZE, "--shutdown-policy=send_complete");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(ShutdownReceiveComplete)
{
    start_iperf_server(PAYLOAD_SIZE, "--shutdown-policy=receive_complete");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(VerifyNone)
{
    start_iperf_server(PAYLOAD_SIZE, "--verify=none");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(VerifyFirst)
{
    start_iperf_server(PAYLOAD_SIZE, "--verify=first");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(VerifyAll)
{
    start_iperf_server(PAYLOAD_SIZE, "--verify=all");

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(RxOnly)
{
    start_iperf_server(PAYLOAD_SIZE,
                       "--mode=rx --shutdown-policy=receive_complete",
                       TO_NXIPERF);

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_CASE(TxOnly)
{
    start_iperf_server(PAYLOAD_SIZE,
                       "--mode=tx --shutdown-policy=send_complete",
                       FROM_NXIPERF);

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(ClientTcp, TcpFixture)

BOOST_AUTO_TEST_CASE(ShutdownSendComplete)
{
    start_peer_server();
    start_iperf_client(PAYLOAD_SIZE);

    io_service_.run();

    wait_for_iperf();
}

BOOST_AUTO_TEST_SUITE_END()

