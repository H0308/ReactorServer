#ifndef __rs_http_request_h__
#define __rs_http_request_h__

#include <regex>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace rs_http_request
{
    class HttpRequest
    {
    public:
        HttpRequest()
            :version_("HTTP/1.1")
        {

        }

        void setHeader(const std::string &key, const std::string &value)
        {
            headers_[key] = value;
        }

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

        void setParam(const std::string &key, const std::string &value)
        {
            params_[key] = value;
        }

        std::string getParam(const std::string &key)
        {
            auto pos = params_.find(key);
            if (pos == params_.end())
                return "";

            return params_[key];
        }

        bool isInParams(const std::string &key)
        {
            auto pos = params_.find(key);
            if (pos == params_.end())
                return false;

            return true;
        }

        void setMethod(const std::string &m)
        {
            method_ = m;
        }

        void clear()
        {
            method_.clear();
            version_ = "HTTP/1.1";
            path_.clear();
            body_.clear();
            headers_.clear();
            params_.clear();
        }

        size_t getContentLength()
        {
            if (!isInHeaders("Content-Length"))
                return 0;

            return std::stol(headers_["Content-Length"]);
        }

        bool isKeepAlive()
        {
            if (isInHeaders("Connection") && headers_["Connection"] == "keep-alive")
                return true;

            return false;
        }

        std::string getMethod()
        {
            return method_;
        }

        void setPath(const std::string &p)
        {
            path_ = p;
        }

        std::filesystem::path getPath()
        {
            return path_;
        }

        void setVersion(const std::string &v)
        {
            version_ = v;
        }

        std::string getVersion()
        {
            return version_;
        }

        void setBody(const std::string &b)
        {
            body_ = b;
        }

        std::string &getBody()
        {
            return body_;
        }

    private:
        std::string method_;                                   // 请求方法
        std::filesystem::path path_;                           // 请求资源路径
        std::string version_;                                  // 协议版本
        std::unordered_map<std::string, std::string> headers_; // 请求头
        std::unordered_map<std::string, std::string> params_;  // 请求参数
        std::string body_;                                     // 请求体
    };
}

#endif