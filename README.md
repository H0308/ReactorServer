# 仿Muduo的高性能服务器（服务端）

## 简介

项目介绍：一个使用C++语言开发的、仿照Muduo库实现的轻量级、高性能网络库。本项目旨在为开发者提供一个简洁、高效、易于扩展的底层网络通信框架，核心基于Reactor事件驱动模型，支持多线程并发处理，能够快速构建各类TCP服务，例如[回显客户端消息的Echo服务器](https://github.com/H0308/ReactorServer/tree/main/reactor_server/demo/echo_server)、[基于应用层HTTP协议的HTTP服务器](https://github.com/H0308/ReactorServer/tree/main/reactor_server/demo/http_server)

项目特点：
1.  基于Reactor的事件驱动模型：核心采用`EventLoop` + `Poller` + `Channel`的Reactor模型，通过事件回调机制高效处理I/O事件、定时器事件和其他任务，实现了非阻塞的异步操作。
2.  多线程并发模型：采用`one loop per thread` + `LoopThreadPool`的线程模型，主线程负责接收新连接，并通过轮询算法分发给子线程（I/O线程）处理，充分利用多核CPU性能，避免了锁竞争带来的开销。
3.  封装完善的网络组件：提供了`Socket`、`Acceptor`、`Connection`、`Buffer`等核心网络组件的封装，简化了网络编程的复杂性。同时提供了`TcpServer`类，用户只需设置回调函数即可快速搭建一个功能完整的TCP服务器。
4.  连接超时管理机制：内置基于时间轮（`TimingWheel`）的定时器，能够高效地管理大量连接的生命周期，自动处理非活跃连接，提高了系统的健壮性和资源利用率。
5.  内置HTTP协议支持：提供了基础的HTTP服务器实现，支持处理`GET`、`POST`、`PUT`、`DELETE`等常见请求方法，可以作为Web服务的底层框架。

## 资源准备

1. C++17及以上
2. Boost库
3. spdlog日志库

> 项目资源准备自行完成

## 回显客户端消息的Echo服务器

```shell
git clone https://github.com/H0308/ReactorServer.git
cd ReactorServer/reactor_server/demo/echo_server
# 先修改Makefile中有关资源路径的配置
./server
./client
```

## 基于应用层HTTP协议的HTTP服务器

```shell
git clone https://github.com/H0308/ReactorServer.git
cd ReactorServer/reactor_server/demo/http_server
# 先修改Makefile中有关资源路径的配置
./server
```

可以根据[示例文件](https://github.com/H0308/ReactorServer/blob/main/reactor_server/demo/http_server/server.cc)自行搭建一个HTTP服务器，支持处理`GET`、`POST`、`PUT`和`DELETE`方法

## 项目模块介绍
          
### 基础模块 (`base/`)
- `error.h`：错误处理模块，提供统一的错误码和异常处理机制
- `log.h`：日志系统，用于记录服务器运行时的各类信息
- `uuid_generator.h`：UUID生成器，为每个连接生成唯一标识符

### 网络模块 (`net/`)

#### 核心组件

- `socket.h`：Socket封装，提供TCP套接字的基础操作
- `buffer.h`：缓冲区管理，实现高效的数据读写和缓存
- `channel.h`：事件通道，负责文件描述符的事件分发
- `poller.h`：事件轮询器，基于epoll实现的I/O多路复用
- `connection.h`：连接管理，处理TCP连接的生命周期
- `acceptor.h`：连接接收器，处理新连接的建立

#### 多线程支持
- `event_loop_lock_queue.h`：事件循环队列，确保线程安全的事件处理
- `loop_thread.h`：事件循环线程，实现one loop per thread模型
- `loop_thread_pool.h`：线程池管理，提供多线程并发处理能力

#### 服务器框架

- `tcp_server.h`：TCP服务器封装，提供完整的服务器功能
- `timing_wheel.h`：时间轮算法实现，用于管理连接超时
- `schedule_task.h`：任务调度器，处理定时任务
- `signal_ign.h`：信号处理，确保服务器稳定运行

#### HTTP协议支持 (`net/http/`)

- `http_server.h`：HTTP服务器实现
- `http_request.h`：HTTP请求解析
- `http_response.h`：HTTP响应生成
- `http_context.h`：HTTP上下文管理

##### HTTP工具类 (`net/http/utils/`)

- `common_op.h`：通用操作工具
- `file_op.h`：文件操作处理
- `url_op.h`：URL解析和处理
- `info_get.h`：信息获取工具