#include <reactor_server/net/signal_ign.h>
#include <sstream>
#include <reactor_server/net/http/http_server.h>

using namespace log_system;

const std::string default_base_dir = "./wwwroot";

void getHandler(rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp)
{
    LOG(Level::Info, "收到GET请求");
    // LOG(Level::Debug, "响应结果为：{}", resp_str);
    resp.setBody(req.getBody());

    // 模仿超时的业务处理
    // sleep(15);
}

void postHandler(rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp)
{
    LOG(Level::Info, "收到POST请求");
}

void putHandler(rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp)
{
    LOG(Level::Info, "收到PUT请求");
    // 读取大文件保存到当前网站根目录
    rs_file_op::FileOp::writeFile(default_base_dir + "/" + "test.txt", req.getBody());
}

void deleteHandler(rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp)
{
    LOG(Level::Info, "收到DELETE请求");
}

int main()
{
    ENABLE_CONSOLE_LOG();

    rs_http_server::HttpServer server(8080);
    server.setThreadNum(3);
    server.setBaseDir(default_base_dir);
    server.setGetHandler("/get", getHandler);
    server.setPostHandler("/post", postHandler);
    server.setPutHandler("/put", putHandler);
    server.setDeleteHandler("/delete", deleteHandler);
    server.startServer();

    return 0;
}
