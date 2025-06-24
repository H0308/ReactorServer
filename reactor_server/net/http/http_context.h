#ifndef __rs_http_context_h__
#define __rs_http_context_h__

#include <reactor_server/base/log.h>
#include <reactor_server/net/buffer.h>
#include <reactor_server/net/http/http_request.h>
#include <reactor_server/net/http/utils/common_op.h>
#include <reactor_server/net/http/utils/url_op.h>

namespace rs_http_context
{
    using namespace log_system;

    enum class ReqRecvStatus
    {
        RecvLine,
        RecvHeader,
        RecvBody,
        RecvOk,
        RecvError
    };

    const int max_request_line_size = 8192; // 8MB数据
    const std::string params_sep = "&";
    const std::string key_value_sep = "=";
    const std::string header_sep = ": ";

    class HttpContext
    {
    public:
        HttpContext()
            : response_status_(200), recv_status_(ReqRecvStatus::RecvLine)
        {
        }

        int getResponseStatus()
        {
            return response_status_;
        }

        ReqRecvStatus getRecvStatus()
        {
            return recv_status_;
        }

        rs_http_request::HttpRequest &getRequest()
        {
            return request_;
        }

        void constructHttpRequest(rs_buffer::Buffer &buf)
        {
            // 此处不需要使用break，因为处理完一个阶段要接着向下处理
            // 如果有一处失败，会因为状态检测失败而无法进入下一个阶段
            switch (recv_status_)
            {
            case ReqRecvStatus::RecvLine:
                handleRequestLine(buf);
            case ReqRecvStatus::RecvHeader:
                handleRequestLine(buf);
            case ReqRecvStatus::RecvBody:
                handleRequestLine(buf);
            }
        }

        void clear()
        {
            response_status_ = 200;
            recv_status_ = ReqRecvStatus::RecvLine;
            request_.clear();
        }

    private:
        // 处理缓冲区中关于请求行的数据
        // 确保缓冲区数据存在一行数据，并且该数据不会过大
        bool handleRequestLine(rs_buffer::Buffer &buf)
        {
            if (recv_status_ != ReqRecvStatus::RecvLine)
                return false;

            // 获取请求行的数据
            std::string line;
            buf.readLine_move(line);
            // 如果数据为空，则分为两种情况：
            // 1. 不足一行数据，直接返回
            // 2. 数据过大，不处理
            if (line.size() == 0)
            {
                if (buf.getReadableSize() > max_request_line_size)
                {
                    // 请求行数据过大
                    response_status_ = 414; // URI Too Long
                    recv_status_ = ReqRecvStatus::RecvError;
                    return false;
                }

                // 数据不足一行，返回真表示本次处理结束
                return true;
            }

            // 如果获取到的一行数据过大，也返回错误信息
            if (line.size() > max_request_line_size)
            {
                // 请求行数据过大
                response_status_ = 414; // URI Too Long
                recv_status_ = ReqRecvStatus::RecvError;
                return false;
            }

            // 此时可以进行处理
            return parseHttpRequestLine(line);
        }

        // 使用正则表达式针对每一个字段进行获取
        std::smatch getContentFromRequestLine(const std::string &line)
        {
            // (\?(.*))*
            // 表示满足\?(.*)的字符串可以有1次或者多次
            // std::regex expr(R"((GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE|CONNECT) (/[^?]*)(\?(.*))* (HTTP/1.[01])(?:\r\n|\n)?)");
            // 但是上面的写法会有问题，当满足的字符串不存在时，(.*)与()*会产生两个空的捕获组
            // 所以可以使用(?:...)去掉外部的捕获组
            // 使用std::regex::icase选项忽略大小写
            std::regex expr(
                R"((GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE|CONNECT) (/[^?]*)(?:\?(.*))* (HTTP/1.[01])(?:\r\n|\n)?)", std::regex::icase);

            std::smatch results;
            if (!std::regex_match(line, results, expr))
                return std::smatch();

            return results;
        }

        // 将请求行字段设置到HttpRequest对象中
        bool parseHttpRequestLine(const std::string &line)
        {
            // 针对请求行进行字段的获取
            std::smatch matches = getContentFromRequestLine(line);
            if (matches.size() == 0)
            {
                // 请求行数据过大
                response_status_ = 400; // Bad Request
                recv_status_ = ReqRecvStatus::RecvError;
                LOG(Level::Warning, "请求行字段获取为空，处理失败");
                return false;
            }

            // 设置属性
            std::string method = matches[1].str();
            // 将请求方式全部修改为大写字母
            std::transform(method.begin(), method.end(), method.begin(), ::toupper);
            request_.setMethod(method);
            std::string decode_path;
            // 路径部分并没有规定空格转加号
            rs_url_op::UrlOp::urlDecode(decode_path, matches[2].str());
            request_.setPath(decode_path);
            request_.setBody(matches[4].str());

            // 处理请求参数
            // 按照&对参数部分进行分割
            std::vector<std::string> params;
            bool ret = rs_common_op::CommonOp::split(params, matches[3].str(), params_sep);
            if (!ret)
            {
                response_status_ = 400; // Bad Request
                recv_status_ = ReqRecvStatus::RecvError;
                LOG(Level::Warning, "参数错误，请求处理失败");
                return false;
            }
            for (auto &str : params)
            {
                std::vector<std::string> out;
                // 按照=进行分割
                bool ret = rs_common_op::CommonOp::split(out, str, key_value_sep);
                if (!ret)
                {
                    response_status_ = 400; // Bad Request
                    recv_status_ = ReqRecvStatus::RecvError;
                    LOG(Level::Warning, "参数错误，请求处理失败");
                    return false;
                }

                std::string decode_param1;
                if (rs_url_op::UrlOp::urlDecode(decode_param1, out[0]))
                {
                    response_status_ = 400; // Bad Request
                    recv_status_ = ReqRecvStatus::RecvError;
                    LOG(Level::Warning, "参数解码错误，请求处理失败");
                    return false;
                }

                std::string decode_param2;
                if (rs_url_op::UrlOp::urlDecode(decode_param2, out[1]))
                {
                    response_status_ = 400; // Bad Request
                    recv_status_ = ReqRecvStatus::RecvError;
                    LOG(Level::Warning, "参数解码错误，请求处理失败");
                    return false;
                }

                request_.setParam(decode_param1, decode_param2);
            }

            recv_status_ = ReqRecvStatus::RecvHeader;

            return true;
        }

        // 处理缓冲区中关于请求头的数据
        bool handleRequestHeader(rs_buffer::Buffer &buf)
        {
            if (recv_status_ != ReqRecvStatus::RecvHeader)
                return false;

            while (true)
            {
                // 获取请求行的数据
                std::string line;
                buf.readLine_move(line);
                if (line.size() == 0)
                {
                    if (buf.getReadableSize() > max_request_line_size)
                    {
                        response_status_ = 414; // URI Too Long
                        recv_status_ = ReqRecvStatus::RecvError;
                        return false;
                    }

                    return true;
                }

                if (line.size() > max_request_line_size)
                {
                    response_status_ = 414; // URI Too Long
                    recv_status_ = ReqRecvStatus::RecvError;
                    return false;
                }

                // 找到空行就结束
                if (line == "\r\n" || line == "\n")
                    break;

                // 处理当前行数据
                bool ret = parseHttpRequestHeader(line);
                if (!ret)
                    return false;
            }

            recv_status_ = ReqRecvStatus::RecvBody;

            return true;
        }

        // 处理请求头字段
        bool parseHttpRequestHeader(const std::string &line)
        {
            std::vector<std::string> key_value;
            bool ret = rs_common_op::CommonOp::split(key_value, line, header_sep);
            if (!ret)
            {
                response_status_ = 400; // Bad Request
                recv_status_ = ReqRecvStatus::RecvError;
                LOG(Level::Warning, "请求头字段参数错误，请求处理失败");
                return false;
            }

            if (key_value.size() == 2)
                request_.setHeader(key_value[0], key_value[1]);
        }

        // 处理缓冲区中关于请求体字段
        bool handleHttpRequestBody(rs_buffer::Buffer &buf)
        {
            if (recv_status_ != ReqRecvStatus::RecvBody)
                return false;

            // 获取请求体大小
            size_t content_length = request_.getContentLength();
            if (content_length == 0)
            {
                // 代表请求体没有数据，处理完毕
                recv_status_ = ReqRecvStatus::RecvOk;
                return true;
            }

            // 获取还需要的实际请求体内容大小
            // 因为当前函数调用可能不是第一次获取请求体内容，而是补充原有请求后续的请求体内容
            size_t rest_length = content_length - request_.getBody().size();
            if (buf.getReadableSize() >= rest_length)
            {
                // 说明至少可以满足当前请求体内容
                recv_status_ = ReqRecvStatus::RecvOk;
                std::string &body = request_.getBody();
                std::string out;
                buf.read_move(out, rest_length);
                body += out;
                return true;
            }

            // 否则当前缓冲区的数据就是小于需要的剩余长度，获取缓冲区所有数据放入请求体
            std::string &body = request_.getBody();
            std::string out;
            body.append(buf.getReadPos(), buf.getReadableSize());
            buf.moveReadPtr(buf.getReadableSize());
            // 此时不需要更新状态，因为还需要后续继续读取内容放入请求体
            return true;
        }

    private:
        int response_status_;                  // 响应状态码
        ReqRecvStatus recv_status_;            // 请求接收状态
        rs_http_request::HttpRequest request_; // HTTP请求对象
    };
}

#endif