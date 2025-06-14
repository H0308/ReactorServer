#include <reactor_server/demo/echo_server/echo_server.h>

int main()
{
    echo_server::EchoServer server(8080);
    server.start();

    return 0;
}
