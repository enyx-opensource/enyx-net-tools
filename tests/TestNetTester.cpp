#define BOOST_TEST_MODULE NetTester

#include <cstdint>
#include <vector>
#include <iostream>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/process.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

namespace p = boost::process;
namespace ip = boost::asio::ip;

static constexpr std::size_t PAYLOAD_SIZE = 1024 * 1024;

struct TcpFixture
{
    enum Direction { TO_NET_TESTER, FROM_NET_TESTER, BOTH };

    TcpFixture()
        : io_service_(),
          peer_socket_(io_service_),
          peer_acceptor_(io_service_),
          net_tester_server_stdout_(io_service_),
          net_tester_(),
          endpoint_(ip::address::from_string("127.0.0.1"), 1234),
          net_tester_buffer_(),
          peer_buffer_(1024 * 1024),
          requested_size_(),
          direction_()
    { }

    void
    start_net_tester_server(std::size_t requested_size,
                       const std::string & args = std::string{},
                       Direction direction = BOTH)
    {
        requested_size_ = requested_size;
        direction_ = direction;

        std::ofstream{"net_tester-cmd", std::ofstream::trunc}
                << " --listen=" << endpoint_.address()
                                << ":"
                                << endpoint_.port()
                << " --size=" << requested_size_ << "B"
                << " --max-datagram-size=1B:32KiB"
                << " " << args;

        net_tester_ = p::child{NET_TESTER_BINARY_PATH 
                               " --configuration-file=net_tester-cmd",
                               p::std_in < p::null,
                               p::std_out > net_tester_server_stdout_,
                               p::std_err > stderr,
                               io_service_};

        BOOST_TEST_CHECKPOINT("net_tester client started");

        async_read_net_tester_output();
    }

    void
    start_net_tester_client(std::size_t requested_size,
                       const std::string & args = std::string{})
    {
        requested_size_ = requested_size;

        std::ofstream{"net_tester-cmd", std::ofstream::trunc}
                << " --connect=127.0.0.1:0:"
                << endpoint_.address() << ":" << endpoint_.port()
                << " --size=" << requested_size_ << "B"
                << " --max-datagram-size=1B:32KiB"
                << " " << args;

        net_tester_ = p::child{NET_TESTER_BINARY_PATH
                               " --configuration-file=net_tester-cmd",
                               p::std_in < p::null,
                               p::std_out > net_tester_server_stdout_,
                               p::std_err > stderr,
                               io_service_};

        BOOST_TEST_CHECKPOINT("net_tester client started");

        async_read_net_tester_output();
    }

    void
    start_peer_server(Direction direction = BOTH)
    {
        direction_ = direction;

        peer_acceptor_.open(endpoint_.protocol());
        ip::tcp::acceptor::reuse_address reuse_address{true};
        peer_acceptor_.set_option(reuse_address);
        peer_acceptor_.bind(endpoint_);
        peer_acceptor_.listen();
        peer_acceptor_.async_accept(peer_socket_,
                                    boost::bind(&TcpFixture::on_peer_connect, this, _1));

        std::cout << "Waiting for net_tester to connect" << std::endl;
    }

    void
    wait_for_net_tester()
    {
        net_tester_.wait();
        BOOST_REQUIRE_EQUAL(0, net_tester_.exit_code());
    }

    void
    async_read_net_tester_output()
    {
        boost::asio::async_read_until(net_tester_server_stdout_,
                                      net_tester_buffer_,
                                      '\n',
                                      boost::bind(&TcpFixture::on_net_tester_output_receive, this, _1, _2));
    }

    void
    on_net_tester_output_receive(const boost::system::error_code & failure,
                                 std::size_t bytes_received)
    {
        if (! failure)
        {
            std::string line;
            {
                std::istream is(&net_tester_buffer_);
                std::getline(is, line);
            }

            if (! line.empty())
                std::cout << "enyx-net-tester: " << line << std::endl;

            if (line.find("Started") == 0 && ! peer_socket_.is_open())
                peer_socket_.async_connect(endpoint_,
                                           boost::bind(&TcpFixture::on_peer_connect, this, _1));

            async_read_net_tester_output();
        }
    }

    void
    on_peer_connect(const boost::system::error_code & failure)
    {
        BOOST_REQUIRE(! failure);
        if (direction_ == FROM_NET_TESTER)
        {
            peer_socket_.shutdown(peer_socket_.shutdown_send);
            async_read_rx_peer();
        }
        else if (direction_ == TO_NET_TESTER)
            async_write_tx_peer();
        else
            async_read_rxtx_peer();

        std::cout << "Connected to enyx-net-tester" << std::endl;
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
    p::async_pipe net_tester_server_stdout_;
    p::child net_tester_;
    ip::tcp::endpoint endpoint_;
    boost::asio::streambuf net_tester_buffer_;
    std::vector<std::uint8_t> peer_buffer_;
    std::size_t requested_size_;
    Direction direction_;
};

BOOST_FIXTURE_TEST_SUITE(ServerTcp, TcpFixture)

BOOST_AUTO_TEST_CASE(ShutdownSendComplete)
{
    start_net_tester_server(PAYLOAD_SIZE, "--shutdown-policy=send_complete");

    io_service_.run();

    wait_for_net_tester();
}

BOOST_AUTO_TEST_CASE(ShutdownReceiveComplete)
{
    start_net_tester_server(PAYLOAD_SIZE, "--shutdown-policy=receive_complete");

    io_service_.run();

    wait_for_net_tester();
}

BOOST_AUTO_TEST_CASE(VerifyNone)
{
    start_net_tester_server(PAYLOAD_SIZE, "--verify=none");

    io_service_.run();

    wait_for_net_tester();
}

BOOST_AUTO_TEST_CASE(VerifyFirst)
{
    start_net_tester_server(PAYLOAD_SIZE, "--verify=first");

    io_service_.run();

    wait_for_net_tester();
}

BOOST_AUTO_TEST_CASE(VerifyAll)
{
    start_net_tester_server(PAYLOAD_SIZE, "--verify=all");

    io_service_.run();

    wait_for_net_tester();
}

BOOST_AUTO_TEST_CASE(RxOnly)
{
    start_net_tester_server(PAYLOAD_SIZE,
                       "--mode=rx --shutdown-policy=receive_complete",
                       TO_NET_TESTER);

    io_service_.run();

    wait_for_net_tester();
}

BOOST_AUTO_TEST_CASE(TxOnly)
{
    start_net_tester_server(PAYLOAD_SIZE,
                       "--mode=tx --shutdown-policy=send_complete",
                       FROM_NET_TESTER);

    io_service_.run();

    wait_for_net_tester();
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(ClientTcp, TcpFixture)

BOOST_AUTO_TEST_CASE(ShutdownSendComplete)
{
    start_peer_server();
    start_net_tester_client(PAYLOAD_SIZE);

    io_service_.run();

    wait_for_net_tester();
}

BOOST_AUTO_TEST_SUITE_END()

struct UdpFixture
{
    enum Direction { TO_NET_TESTER, FROM_NET_TESTER, BOTH };

    UdpFixture()
        : io_service_(),
          peer_socket_(io_service_),
          net_tester_server_stdout_(io_service_),
          net_tester_(),
          remote_endpoint_(ip::address::from_string("127.0.0.1"), 1235),
          local_endpoint_(ip::address::from_string("127.0.0.1"), 1234),
          net_tester_buffer_(),
          peer_buffer_(65535),
          requested_size_(),
          direction_()
    { }

    void
    start_net_tester_client(std::size_t requested_size,
                       const std::string & args = std::string{})
    {
        requested_size_ = requested_size;

        std::ofstream{"net_tester-cmd", std::ofstream::trunc}
                         << " --connect="
                         << local_endpoint_.address() << ":"
                         << local_endpoint_.port() << ":"
                         << remote_endpoint_.address() << ":"
                         << remote_endpoint_.port()
                         << " --size=" << requested_size_ << "B"
                         << " " << args;

        net_tester_ = p::child{NET_TESTER_BINARY_PATH
                               " --configuration-file=net_tester-cmd",
                               p::std_in < p::null,
                               p::std_out > net_tester_server_stdout_,
                               p::std_err > stderr,
                               io_service_};

        BOOST_TEST_CHECKPOINT("net_tester client started");

        async_read_net_tester_output();
    }

    void
    start_peer_server(Direction direction = BOTH)
    {
        direction_ = direction;

        peer_socket_.open(local_endpoint_.protocol());
        ip::udp::socket::reuse_address reuse_address{true};
        peer_socket_.set_option(reuse_address);
        peer_socket_.bind(local_endpoint_);
        peer_socket_.connect(remote_endpoint_);

        if (direction_ == FROM_NET_TESTER)
            async_read_rx_peer();
        else if (direction_ == TO_NET_TESTER)
            async_write_tx_peer();
        else
            async_read_rxtx_peer();

        std::cout << "Waiting for net_tester to send data" << std::endl;
    }

    void
    wait_for_net_tester()
    {
        net_tester_.wait();
        BOOST_REQUIRE_EQUAL(0, net_tester_.exit_code());
    }

    void
    async_read_net_tester_output()
    {
        boost::asio::async_read_until(net_tester_server_stdout_,
                                      net_tester_buffer_,
                                      '\n',
                                      boost::bind(&UdpFixture::on_net_tester_output_receive, this, _1, _2));
    }

    void
    on_net_tester_output_receive(const boost::system::error_code & failure,
                                 std::size_t bytes_received)
    {
        if (! failure)
        {
            std::string line;
            {
                std::istream is(&net_tester_buffer_);
                std::getline(is, line);
            }

            if (! line.empty())
                std::cout << "enyx-net-tester: " << line << std::endl;

            async_read_net_tester_output();
        }
    }

    void
    async_read_rxtx_peer()
    {
        peer_socket_.async_receive(boost::asio::buffer(peer_buffer_),
                                   boost::bind(&UdpFixture::on_peer_rxtx_receive, this, _1, _2));
    }

    void
    on_peer_rxtx_receive(const boost::system::error_code & failure,
                         std::size_t bytes_received)
    {
        BOOST_REQUIRE(! failure);

        peer_socket_.async_send(boost::asio::buffer(peer_buffer_, bytes_received),
                                boost::bind(&UdpFixture::on_peer_rxtx_sent, this, _1, _2));
    }

    void
    on_peer_rxtx_sent(const boost::system::error_code & failure,
                      std::size_t bytes_sent)
    {
        BOOST_REQUIRE(! failure);

        BOOST_REQUIRE_LE(bytes_sent, requested_size_);
        requested_size_ -= bytes_sent;

        if (requested_size_)
            async_read_rxtx_peer();
    }

    void
    async_read_rx_peer()
    {
        peer_socket_.async_receive(boost::asio::buffer(peer_buffer_),
                                   boost::bind(&UdpFixture::on_peer_rx_receive, this, _1, _2));
    }

    void
    on_peer_rx_receive(const boost::system::error_code & failure,
                       std::size_t bytes_received)
    {
        BOOST_REQUIRE(! failure);

        BOOST_REQUIRE_LE(bytes_received, requested_size_);
        requested_size_ -= bytes_received;

        if (requested_size_)
            async_read_rx_peer();
    }

    void
    async_write_tx_peer()
    {
        const std::size_t size = std::min(peer_buffer_.size(), requested_size_);

        peer_socket_.async_send(boost::asio::buffer(peer_buffer_, size),
                                boost::bind(&UdpFixture::on_peer_tx_sent, this, _1, _2));
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
    }

    boost::asio::io_service io_service_;
    ip::udp::socket peer_socket_;
    p::async_pipe net_tester_server_stdout_;
    p::child net_tester_;
    ip::udp::endpoint remote_endpoint_;
    ip::udp::endpoint local_endpoint_;
    boost::asio::streambuf net_tester_buffer_;
    std::vector<std::uint8_t> peer_buffer_;
    std::size_t requested_size_;
    Direction direction_;
};

BOOST_FIXTURE_TEST_SUITE(ClientUdp, UdpFixture)

BOOST_AUTO_TEST_CASE(RxTx, * boost::unit_test::disabled())
{
    start_peer_server();
    start_net_tester_client(1024, "--protocol=udp");

    io_service_.run();

    wait_for_net_tester();
}

BOOST_AUTO_TEST_SUITE_END()
