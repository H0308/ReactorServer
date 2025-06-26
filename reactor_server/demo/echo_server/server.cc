#include <reactor_server/net/signal_ign.h>
#include <reactor_server/demo/echo_server/echo_server.h>

using namespace rs_log_system;

int main()
{
    ENABLE_CONSOLE_LOG();

    echo_server::EchoServer server(8080);
    server.start();

    return 0;
}
