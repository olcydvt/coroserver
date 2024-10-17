#include <coroutine>
#include <iostream>
#include <thread>
#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <latch>


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

        std::suspend_always yield_value(int val) {
            this->value = val;
            return std::suspend_always{};
        }

        double get_value() const noexcept { return value; }
        void set_value(double val) noexcept { value = val; }

    private:
        double value{3.14};
    };

    std::coroutine_handle<promise_type> h_;

    ReturnObject(std::coroutine_handle<promise_type> h) : h_{h} {
    }

    operator std::coroutine_handle<promise_type>() { return h_; }

    int get_value() const {
        return h_.promise().get_value();
    }

    ~ReturnObject() {
        h_.destroy();
    }
};

class TcpCoroSession {
    asio::basic_stream_socket<asio::ip::tcp> conn;
    std::thread thread;
    std::array<char, 1024> buffer;
public:
    TcpCoroSession(asio::basic_stream_socket<asio::ip::tcp> &&_conn) : conn(std::move(_conn)) {
    }

    void run() {
        thread = std::thread(&TcpCoroSession::do_work, this);
    }

    void do_work() {
        while (true) {
            size_t len = conn.read_some(asio::buffer(buffer, 1024));
            std::string data{&buffer[0], len};
            std::cout << data << std::endl;
            std::cout << "do work thread id: " << std::this_thread::get_id() << std::endl;
        }

    }
};

class TcpCoroSessionHandler {
    std::vector<std::unique_ptr<TcpCoroSession> > sessions;

public:
    void store_session(std::unique_ptr<TcpCoroSession> session, std::coroutine_handle<ReturnObject::promise_type> h) {
        sessions.push_back(std::move(session));
        auto sess = sessions.rbegin();
        sess->get()->run();
        h.resume();
    }
};

struct TcpCoroAcceptor {
    asio::io_context &io_context_;
    asio::ip::tcp::acceptor acceptor_;
    TcpCoroSessionHandler &tcp_coro_session_handler;

    TcpCoroAcceptor(TcpCoroSessionHandler &_tcp_coro_session_handler, asio::io_context &io_context)
        : io_context_(io_context)
          , tcp_coro_session_handler(_tcp_coro_session_handler)
          , acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 65535)) {
    }

    void await_suspend(std::coroutine_handle<ReturnObject::promise_type> h) {
        auto conn = acceptor_.accept();
        auto session = std::make_unique<TcpCoroSession>(std::move(conn));
        std::cout << "awaiting suspend call from " << "thread id: " << std::this_thread::get_id() << std::endl;
        tcp_coro_session_handler.store_session(std::move(session), h);
    }

    void await_resume() {
    }

    bool await_ready() const noexcept { return false; }
};

class TcpCoroServer {
    std::atomic_bool running_{false};
    TcpCoroSessionHandler &tcp_coro_session_handler;
    asio::io_context &io_context_;

public:
    TcpCoroServer(asio::io_context &io_context, TcpCoroSessionHandler &_tcp_coro_session_handler)
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

private:
};


int main(int argc, char **argv) {
    std::latch _latch{1};
    asio::io_context io_context_;
    TcpCoroSessionHandler tcp_coro_session_handler;
    TcpCoroServer server(io_context_, tcp_coro_session_handler);
    server.start();
    io_context_.run();
    _latch.wait();
    return 0;
}
