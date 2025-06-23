#ifndef __rs_http_response_h__
#define __rs_http_response_h__

#include <string>
#include <unordered_map>

namespace rs_http_response
{
    class HttpResponse
    {
    public:
        HttpResponse()
            :HttpResponse(200)
        {}

        HttpResponse(int status)
            :toRedirect_(false), status_(status)
        {
        }

        // 设置响应状态码
        void setStatus(int code)
        {
            status_ = code;
        }

        // 获取响应状态码
        int getStatus()
        {
            return status_;
        }

        // 设置响应正文
        void setBody(const std::string &body, const std::string &type = "text/html")
        {
            body_ = body;
            setHeader("Content-Type", type);
        }

        // 获取响应正文
        std::string getBody()
        {
            return body_;
        }

        // 添加响应头
        void setHeader(const std::string &key, const std::string &value)
        {
            headers_[key] = value;
        }

        // 获取响应头
        std::string getHeader(const std::string &key)
        {
            auto pos = headers_.find(key);
            if (pos == headers_.end())
                return "";

            return headers_[key];
        }

        bool isInHeaders(const std::string &key)
        {
            auto pos = headers_.find(key);
            if (pos == headers_.end())
                return false;

            return true;
        }

        // 启用重定向
        void enableRedirect(const std::string &url, int code = 302)
        {
            status_ = code;
            toRedirect_ = true;
            redirect_url_ = url;
        }

        // 获取重定向地址
        std::string getRedirectUrl()
        {
            return redirect_url_;
        }

        // 是否启用重定向
        bool isRedirectEnabled()
        {
            return toRedirect_;
        }

        // 是否保持长连接
        bool isKeepAlive()
        {
            if (isInHeaders("Connection") && headers_["Connection"] == "keep-alive")
                return true;
                
            return false;
        }

        void clear()
        {
            status_ = 200;
            toRedirect_ = false;
            redirect_url_.clear();
            body_.clear();
            headers_.clear();
        }
    private:
        int status_; // 响应状态码
        bool toRedirect_; // 是否启用重定向
        std::string redirect_url_; // 重定向地址
        std::string body_; // 响应正文
        std::unordered_map<std::string, std::string> headers_; // 请求头
    };
}

#endif