#include <cstdlib>
#include <sstream>
#include <iostream>

#include <boost/process.hpp>

namespace p = boost::process;

static void
wait_for_listening(p::ipstream & process_stdout)
{
    std::string line;
    while (std::getline(process_stdout, line))
        if (line.find("Waiting") != std::string::npos)
            break;
}

int
main(int argc, char **argv)
{
    if (argc != 4)
        return EXIT_FAILURE;

    std::string const iperf_binary{argv[1]};

    p::group process_group;

    std::ostringstream iperf_server_cmd;
    iperf_server_cmd << iperf_binary
                     << " --listen=127.0.0.1:1234 " << argv[2];
    p::ipstream iperf_server_stdout;
    p::ipstream iperf_server_stderr;
    p::child iperf_server(iperf_server_cmd.str(),
                          p::std_in < p::null,
                          p::std_out > iperf_server_stdout,
                          p::std_err > iperf_server_stderr,
                          process_group);

    wait_for_listening(iperf_server_stdout);

    std::ostringstream iperf_client_cmd;
    p::ipstream iperf_client_stderr;
    iperf_client_cmd << iperf_binary
                     << " --connect=127.0.0.1:1234 " << argv[3];
    p::child iperf_client(iperf_client_cmd.str(),
                          p::std_in < p::null,
                          p::std_out > p::null,
                          p::std_err > iperf_client_stderr,
                          process_group);

    bool failure = false;

    iperf_client.wait();
    if (iperf_client.exit_code())
    {
        failure = true;
        std::cerr << "client failed: " << iperf_client_stderr.rdbuf();
    }

    iperf_server.wait();
    if (iperf_server.exit_code())
    {
        failure = true;
        std::cerr << "server failed: " << iperf_server_stderr.rdbuf();
    }

    return failure ? EXIT_SUCCESS : EXIT_SUCCESS;
}

