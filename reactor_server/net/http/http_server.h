#ifndef __rs_http_server_h__
#define __rs_http_server_h__

#include <string>
#include <vector>
#include <filesystem>
#include <reactor_server/net/tcp_server.h>
#include <reactor_server/net/http/http_response.h>
#include <reactor_server/net/http/http_context.h>
#include <reactor_server/net/http/utils/common_op.h>
#include <reactor_server/net/http/utils/file_op.h>

namespace rs_http_server
{
    using namespace rs_log_system;

    const int default_timeout = 10;

    class HttpServer
    {
    public:
        using handler_t = std::function<void(rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp)>;
        using regex_handler_pair_t = std::pair<std::regex, handler_t>;

        HttpServer(int port, uint32_t timeout = default_timeout)
            : server_(port)
        {
            server_.setConnectedCallback(std::bind(&HttpServer::onConnected, this, std::placeholders::_1));
            server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
            server_.setOuterCloseCallback(std::bind(&HttpServer::onClose, this, std::placeholders::_1));
            server_.enableTimeoutRelease(timeout);
        }

        // 设置GET请求处理映射
        void setGetHandler(const std::string &reg, const handler_t &handler)
        {
            get_mapping_.emplace_back(reg, handler);
        }

        // 设置POST请求处理映射
        void setPostHandler(const std::string &reg, const handler_t &handler)
        {
            post_mapping_.emplace_back(reg, handler);
        }

        // 设置PUT请求处理映射
        void setPutHandler(const std::string &reg, const handler_t &handler)
        {
            put_mapping_.emplace_back(reg, handler);
        }

        // 设置DELETE请求处理映射
        void setDeleteHandler(const std::string &reg, const handler_t &handler)
        {
            delete_mapping_.emplace_back(reg, handler);
        }

        // 设置根目录
        void setBaseDir(const std::filesystem::path &path)
        {
            assert(rs_file_op::FileOp::isDirectory(path));
            base_dir_ = path;
        }

        // 设置线程数量
        void setThreadNum(int num)
        {
            server_.setThreadNum(num);
        }

        // 启动服务器
        void startServer()
        {
            server_.start();
        }

    private:
        // 静态资源处理
        void staticResourceHandler(rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp)
        {
            std::filesystem::path req_path = req.getPath();
            std::filesystem::path real_path = base_dir_ / req_path;
            // 如果请求路径为/，则直接返回根目录
            // 当右操作数是绝对路径时（以/开头），它会替换左操作数，而不是拼接。这是std::filesystem::path的标准行为
            if(req_path.string() == "/")
                real_path = base_dir_.string() + "/";
            else if (!req_path.empty() && req_path.string()[0] == '/')
                real_path = base_dir_.string() + req_path.string();
            if (real_path.string().back() == '/')
                real_path /= "index.html";
            // 此时请求中一定是静态资源
            // 读取文件内容
            std::string body;
            bool ret = rs_file_op::FileOp::readFile(real_path, body);
            if (!ret)
                return;

            resp.setBody(body, rs_info_get::InfoGet::getMimeType(rs_file_op::FileOp::getExtensionName(real_path)));
        }

        // 动态资源处理
        void dynamicResourceHandler(rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp, std::vector<regex_handler_pair_t> &router)
        {
            for (auto &pair : router)
            {
                std::string path = req.getPath().string();
                std::smatch matches;
                // 正则匹配
                if (std::regex_match(path, matches, pair.first))
                {
                    pair.second(req, resp);
                    return;
                }
            }

            resp.setStatus(404);
        }

        // 构建错误响应
        void constructErrorResponse(rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp, int code)
        {
            // 判断根目录是否有404.html页面，如果没有，就构建默认的404页面，否则就使用已有的404.html页面
            resp.setStatus(code);
            bool ret = false;
            std::string body;
            if (!base_dir_.empty())
            {
                std::filesystem::path not_found_file = base_dir_ / "404.html";
                ret = rs_file_op::FileOp::readFile(not_found_file, body);
            }
            if (!ret)
            {
                // 不存在对应的404页面，返回默认值
                body += "<html>";
                body += "<head>";
                body += "<meta http-equiv='Content-Type' content='text/html;charset=utf-8'>";
                body += "</head>";
                body += "<body>";
                body += "<h1>";
                body += std::to_string(resp.getStatus());
                body += "</h1>";
                body += "<p>";
                body += rs_info_get::InfoGet::getStatusDesc(resp.getStatus());
                body += "</p>";
                body += "</body>";
                body += "</html>";
            }

            resp.setBody(body);
            resp.setHeader("Content-Length", std::to_string(body.size()));
        }

        // 发送HTTP响应
        void sendResponse(const rs_connection::Connection::ptr &con, rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp)
        {
            // 设置长连接或者短连接属性
            if (req.isKeepAlive())
                resp.setHeader("Connection", "keep-alive");
            else
                resp.setHeader("Connection", "close");

            // 设置内容MIME和内容大小
            if (!resp.getBody().empty() && !resp.isInHeaders("Content-Length"))
                resp.setHeader("Content-Length", std::to_string(resp.getBody().size()));
            if (!resp.getBody().empty() && !resp.isInHeaders("Content-Type"))
                resp.setHeader("Content-Type", rs_info_get::InfoGet::getMimeType(""));

            // 是否设置重定向
            if (resp.isRedirectEnabled())
                resp.setHeader("Location", resp.getRedirectUrl());

            // 根据HttpResponse组织HTTP响应字符串
            std::string resp_str = resp.constructHttpResponseStr(req);

            // 发送响应
            con->send((void *)(resp_str.c_str()), resp_str.size());
        }

        // 判断是否是静态资源请求
        bool isStaticResourceRequest(rs_http_request::HttpRequest &req)
        {
            // 判断根目录是否存在
            if (base_dir_.empty())
                return false;

            // 判断是否是GET或者HEAD请求
            // 其余请求不处理静态资源返回
            if (req.getMethod() != "GET" && req.getMethod() != "HEAD")
                return false;

            // 判断请求的资源路径是否合法
            if (!rs_common_op::CommonOp::isValidResourcePath(req.getPath()))
                return false;

            std::filesystem::path req_path = req.getPath();
            std::filesystem::path real_path = base_dir_ / req_path;
            // 如果请求路径为/，则直接返回根目录
            // 当右操作数是绝对路径时（以/开头），它会替换左操作数，而不是拼接。这是std::filesystem::path的标准行为
            if(req_path.string() == "/")
                real_path = base_dir_.string() + "/";
            else if (!req_path.empty() && req_path.string()[0] == '/')
                real_path = base_dir_.string() + req_path.string();
            if (real_path.string().back() == '/')
                real_path /= "index.html";
            
            // 判断指定路径是否是普通文件
            if (!rs_file_op::FileOp::isRegularFile(real_path))
                return false;

            return true;
        }

        // 根据请求类型查找映射表
        void getMapping(rs_http_request::HttpRequest &req, rs_http_response::HttpResponse &resp)
        {
            // 默认情况下，认为都是静态资源请求
            if (isStaticResourceRequest(req))
            {
                staticResourceHandler(req, resp);
                return;
            }

            // 否则就是静态资源
            if (req.getMethod() == "GET" || req.getMethod() == "HEAD")
            {
                dynamicResourceHandler(req, resp, get_mapping_);
                return;
            }
            else if (req.getMethod() == "POST")
            {
                dynamicResourceHandler(req, resp, post_mapping_);
                return;
            }
            else if (req.getMethod() == "PUT")
            {
                dynamicResourceHandler(req, resp, put_mapping_);
                return;
            }
            else if (req.getMethod() == "DELETE")
            {
                dynamicResourceHandler(req, resp, delete_mapping_);
                return;
            }

            // 如果既不是静态也不是动态，就设置错误状态码
            resp.setStatus(405);
        }

        // 连接回调
        void onConnected(const rs_connection::Connection::ptr &con)
        {
            LOG(Level::Info, "客户端：{}建立连接", con->getFd());
            // 设置上下文
            con->setContext(rs_http_context::HttpContext());
        }

        // 消息回调
        void onMessage(const rs_connection::Connection::ptr &con, rs_buffer::Buffer &buf)
        {
            while (buf.getReadableSize() > 0)
            {
                // 从any中获取到上下文数据
                rs_http_context::HttpContext *context = std::any_cast<rs_http_context::HttpContext>(&con->getContext());
                // 处理缓冲区中的数据
                context->constructHttpRequest(buf);
                // 获取到HttpRequest对象
                rs_http_request::HttpRequest &req = context->getRequest();
                rs_http_response::HttpResponse resp;
                // 进行请求处理
                if (context->getRecvStatus() == rs_http_context::ReqRecvStatus::RecvError)
                {
                    // 构建错误页面
                    constructErrorResponse(req, resp, context->getResponseStatus());
                    // 发送错误响应
                    sendResponse(con, req, resp);
                    context->clear();
                    buf.moveReadPtr(buf.getReadableSize());
                    con->shutdown();
                    return;
                }

                if (context->getRecvStatus() != rs_http_context::ReqRecvStatus::RecvOk)
                    return; // 未拿到一个完整的HTTP请求
                getMapping(req, resp);
                sendResponse(con, req, resp);
                // 清空上下文，注意上方取得的是HttpContext中关于HttpRequest对象的引用
                // 在下方判断长短连接时需要使用设置的HttpResponse对象进行，而不能使用HttpRequest
                // 因为clear中会对HttpRequest对象进行释放，间接影响了上方拿到的关于HttpRequest引用对象
                context->clear();
                // 短连接时直接关闭连接
                if (!resp.isKeepAlive())
                    con->shutdown();
            }
        }

        void onClose(const rs_connection::Connection::ptr &con)
        {
            LOG(Level::Info, "客户端：{}断开连接", con->getFd());
        }

    private:
        rs_tcp_server::TcpServer server_;
        std::filesystem::path base_dir_;
        // 模版参数1表示一个正则表达式
        // 模版参数2表示映射函数
        std::vector<regex_handler_pair_t> get_mapping_;      // GET请求映射
        std::vector<regex_handler_pair_t> post_mapping_;     // POST请求映射
        std::vector<regex_handler_pair_t> put_mapping_;    // PUT请求映射
        std::vector<regex_handler_pair_t> delete_mapping_; // DELETE请求映射
    };
}

#endif