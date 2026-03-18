#include "network/Connection.h"
#include "common/logger/Logger.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/manager/ConnectionManager.h"
#include "network/session/SessionManager.h"
#include "common/thread/GlobalThreadPool.h"

Connection::tcp::socket &Connection::GetSocket()
{
    return socket_;
}

void Connection::Start()
{
    last_active_ = std::chrono::steady_clock::now();
    DoRead();
}

void Connection::HandlePacket(uint16_t msgId, const char *data, size_t len)
{
    MessageDispatcher::Instance().Dispatch(msgId, this, data, len);
}

void Connection::DoRead()
{
    auto self(shared_from_this());

    socket_.async_read_some(
        boost::asio::buffer(buffer_, sizeof(buffer_)),
        boost::asio::bind_executor(strand_,
                                   [this, self](boost::system::error_code ec, std::size_t length)
                                   {
                                       if (!ec)
                                       {
                                           last_active_ = std::chrono::steady_clock::now();
                                           recv_buffer_.Append(buffer_, length);

                                           parser_.Parse(
                                               recv_buffer_,
                                               [this](uint16_t msgId, const char *data, size_t len)
                                               {
                                                   HandlePacket(msgId, data, len);
                                               });

                                           DoRead();
                                       }
                                       else
                                       {
                                           // 避免重复打印，只有非主动关闭时才记录 WARN
                                           if (ec != boost::asio::error::operation_aborted)
                                           {
                                               LOG_WARN("client read error: {} conn={}", ec.message(), connection_id_);
                                           }
                                           Close();
                                       }
                                   }));
}

void Connection::SendPacket(const Packet &packet)
{
    // 如果连接已关闭，直接拦截
    if (closed_.load())
        return;

    auto data = std::make_shared<std::vector<char>>(packet.Serialize());

    boost::asio::post(strand_, [this, self = shared_from_this(), data]()
                      {
        if (closed_.load()) return;

        // 背压控制：发送队列过长时丢弃新包，保护内存
        if (write_queue_.size() > 1024)
        {
            LOG_WARN("drop packet due to queue limit, session={}", session_id_);
            return;
        }

        bool writing = !write_queue_.empty();
        write_queue_.push_back(data);

        if (!writing)
        {
            DoWrite();
        } });
}

void Connection::DoWrite()
{
    if (closed_.load() || write_queue_.empty())
        return;

    auto data = write_queue_.front();

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(*data),
        boost::asio::bind_executor(strand_,
                                   [this, self = shared_from_this()](boost::system::error_code ec, std::size_t)
                                   {
                                       if (ec)
                                       {
                                           Close();
                                           return;
                                       }

                                       // 🔥 【核心修复】：必须先判空！
                                       // 因为在 async_write 回调排队期间，Close() 可能已经在 strand 中执行并清空了队列
                                       if (!write_queue_.empty())
                                       {
                                           write_queue_.pop_front();
                                       }

                                       // 继续处理队列中的下一个包
                                       if (!write_queue_.empty() && !closed_.load())
                                       {
                                           DoWrite();
                                       }
                                   }));
}

void Connection::Close()
{
    // 原子标记，确保 Close 逻辑只执行一次
    if (closed_.exchange(true))
        return;

    boost::asio::post(strand_, [this, self = shared_from_this()]()
                      {
        // 1. 清空发送队列，防止回调继续触发 DoWrite
        write_queue_.clear();

        // 2. 从管理器移除，断开逻辑层关联
        ConnectionManager::Instance().RemoveConnection(connection_id_);

        // 3. 关闭底层 Socket
        if (socket_.is_open())
        {
            boost::system::error_code ec;
            socket_.shutdown(tcp::socket::shutdown_both, ec);
            socket_.close(ec);
        }

        // 4. 提取 SessionID 用于异步清理
        uint64_t sid = session_id_;
        if (sid != 0)
        {
            LOG_INFO("Connection closed, starting async cleanup for sid={}", sid);
            AsyncCleanup(sid); // 传入提取出的 sid
            session_id_ = 0;
        } });
}

void Connection::AsyncCleanup(uint64_t sid)
{
    // 将重操作丢入全局线程池，避免阻塞网络线程
    GlobalThreadPool::Instance().GetPool().Enqueue([sid]()
                                                   {
        auto session = SessionManager::Instance().GetSession(sid);
        if (session)
        {
            auto actor = session->GetActor();
            if (actor)
            {
                actor->Stop();
                LOG_DEBUG("Actor stopped safely in background. sid={}", sid);
            }
            SessionManager::Instance().RemoveSession(sid);
            LOG_INFO("Session resources released for sid={}", sid);
        } });
}