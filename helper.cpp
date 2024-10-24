#include <coroutine>
#include <iostream>
#include <thread>
#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <latch>
/*
using executor_type = asio::io_context::executor_type;
using socket_type = asio::basic_stream_socket<asio::ip::tcp, executor_type>;
using acceptor_type = asio::basic_socket_acceptor<asio::ip::tcp, executor_type>;

struct ReturnObject {
    struct promise_type {
        ReturnObject get_return_object() {
            auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
            return ReturnObject{handle};
        }

        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        void unhandled_exception() {
        }

        void return_void() noexcept {
        }

        // void return_value(int val) noexcept {
        //     this->value = val;
        // }

        std::suspend_always yield_value(std::string val) {
            this->value = val;
            return std::suspend_always{};
        }

        std::string& get_value() noexcept { return value; }

    private:
        std::string value{};
    };

    std::coroutine_handle<promise_type> h_;

    ReturnObject(std::coroutine_handle<promise_type> h) : h_{h} {
    }

    operator std::coroutine_handle<promise_type>() { return h_; }

    const std::string& get_value() const {
        return h_.promise().get_value();
    }

    void clear_value()  {
        h_.promise().get_value().clear();
    }

    ~ReturnObject() {
        h_.destroy();
    }
};

struct TcpCoroListener {
    asio::io_context &io_context_;

    TcpCoroListener(asio::io_context &io_context)
        : io_context_(io_context) {
    }

    void await_suspend(std::coroutine_handle<ReturnObject::promise_type> h) {
        asio::post(io_context_, [h]() mutable {
            h.resume();
        });
    }

    void await_resume() {
    }

    bool await_ready() const noexcept { return false; }
};

class TcpCoroSession {
    asio::basic_stream_socket<asio::ip::tcp> conn;
    std::thread thread;
    std::array<char, 1024> buffer;
    asio::io_context &io_context_;
public:
    TcpCoroSession(asio::basic_stream_socket<asio::ip::tcp> &&_conn, asio::io_context &io_context)
    : conn(std::move(_conn))
    , io_context_(io_context){
    }

    void run() {
        std::cout << "socket_read call thread id: " << std::this_thread::get_id() << std::endl;
        auto socket_ret_obj = socket_read();
    }

    ReturnObject socket_read() {
        while (true) {
            size_t len = conn.read_some(asio::buffer(buffer, 1024));
            std::string data{&buffer[0], len};
            std::cout << "socket_read result thread id: " << std::this_thread::get_id() << std::endl;
            std::cout << data << std::endl;
            data.clear();
            co_await TcpCoroListener{io_context_};
        }
    }
};

class TcpCoroSessionHandler {
    std::vector<std::unique_ptr<TcpCoroSession> > sessions;

public:
    void store_session(std::unique_ptr<TcpCoroSession> session) {
        session->run();
        sessions.push_back(std::move(session));
    }
};

struct TcpCoroAcceptor {
    asio::io_context &io_context_;
    asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<TcpCoroSessionHandler>  tcp_coro_session_handler_;

    TcpCoroAcceptor(std::shared_ptr<TcpCoroSessionHandler> tcp_coro_session_handler, asio::io_context &io_context)
        : io_context_(io_context)
          , tcp_coro_session_handler_(tcp_coro_session_handler)
          , acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 65535)) {
    }

    void await_suspend(std::coroutine_handle<ReturnObject::promise_type> h) {
        std::jthread([h, acceptor = std::move(acceptor_), handler = tcp_coro_session_handler_ ]() mutable {
            std::cout << "awaiting suspend call from " << "thread id: " << std::this_thread::get_id() << std::endl;
            auto conn = acceptor.accept();
            h.resume();
            auto session = std::make_unique<TcpCoroSession>(std::move(conn), ::io_context_);
            handler->store_session(std::move(session));
        });
    }

    void await_resume() {
    }

    bool await_ready() const noexcept { return false; }
};

class TcpCoroServer {
    std::atomic_bool running_{false};
    std::shared_ptr<TcpCoroSessionHandler>  &tcp_coro_session_handler;
    asio::io_context &io_context_;

public:
    TcpCoroServer(asio::io_context &io_context, std::shared_ptr<TcpCoroSessionHandler> &_tcp_coro_session_handler)
        : io_context_(io_context)
          , tcp_coro_session_handler(_tcp_coro_session_handler) {
    }

    ReturnObject start() {
        running_ = true;
        while (running_) {
            std::cout << "awaiting for a new connection " << "thread id: " << std::this_thread::get_id() << std::endl;
            co_await TcpCoroAcceptor{tcp_coro_session_handler, io_context_};
        }
    }
};
*/

struct  server {
    using executor_type = asio::io_context::executor_type;
    using socket_type = asio::basic_stream_socket<asio::ip::tcp, executor_type>;
    using acceptor_type = asio::basic_socket_acceptor<asio::ip::tcp, executor_type>;
    using work_guard_type = asio::executor_work_guard<executor_type>;
    asio::io_context io_context_;

    acceptor_type _acceptor;
    std::optional<work_guard_type> _work_guard;
    server()
        :_acceptor(io_context_, asio::ip::tcp::endpoint{asio::ip::address_v4::from_string(std::string{"127.0.0.1"}), 25000}) {
    }

    void listen(std::shared_ptr<socket_type> socket_) {

        if (socket_->is_open()) {
            std::error_code ec;
            socket_->set_option(asio::ip::tcp::no_delay{true}, ec);

            std::array<char, 8192> m_buffer;
            auto buf = asio::buffer(m_buffer);
           socket_->async_read_some(buf, [this, buf, socket_](std::error_code ec, std::size_t bytes_transferred) {
               std::cout << std::string((char *)buf.data(), bytes_transferred) << std::endl;
               this->listen(socket_);
           });

        }
    }

    void accept() {
            auto socket_ptr = std::make_shared<socket_type>(io_context_);
            _acceptor.async_accept(*socket_ptr, [self = this, socket_ptr](asio::error_code ec) {
                if (ec) {
                    if (ec == asio::error::operation_aborted) {
                        return;
                    }
                    std::cout << "network error " <<  std::endl;
                    return;
                }
                self->listen(socket_ptr);
                self->accept();
            });
    }
};

int main(int argc, char **argv) {
    server server_;
    server_.accept();

    asio::executor_work_guard<server::executor_type> _work_guard{server_.io_context_.get_executor()};
    server_.io_context_.run();
    return 0;
}
