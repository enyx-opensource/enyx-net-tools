#include <cstdlib>
#include <sstream>

#include <boost/process.hpp>

namespace p = boost::process;

int
main(int argc, char **argv)
{
    if (argc != 4)
        return EXIT_FAILURE;

    std::string const iperf_binary{argv[1]};

    p::group group;

    std::ostringstream iperf_server_cmd;
    iperf_server_cmd << iperf_binary
                     << " --listen=127.0.0.1:1234 " << argv[2];
    p::ipstream iperf_server_stdout;
    p::child iperf_server(iperf_server_cmd.str(),
                             p::std_out > iperf_server_stdout,
                             group);

    for (std::string line; std::getline(iperf_server_stdout, line); )
        if (line.find("Waiting") != std::string::npos)
            break;

    std::ostringstream iperf_client_cmd;
    iperf_client_cmd << iperf_binary
                     << " --connect=127.0.0.1:1234 " << argv[3];
    p::child iperf_client(iperf_client_cmd.str(), group);

    iperf_client.wait();
    iperf_server.wait();

    if (iperf_client.exit_code() || iperf_server.exit_code())
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

