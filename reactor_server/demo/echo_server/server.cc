#include <reactor_server/demo/echo_server/echo_server.h>

using namespace log_system;

int main()
{
    ENABLE_CONSOLE_LOG();

    echo_server::EchoServer server(8080);
    server.start();

    return 0;
}
