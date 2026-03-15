#include "network/Connection.h"
#include "common/logger/Logger.h"
#include "network/dispatcher/MessageDispatcher.h"
#include "network/manager/ConnectionManager.h"
#include "network/session/SessionManager.h"

Connection::tcp::socket &Connection::GetSocket()
{
    return socket_;
}

void Connection::Start()
{
    last_active_ = std::chrono::steady_clock::now();

    auto session = SessionManager::Instance().CreateSession();

    session->BindConnection(shared_from_this());

    session_id_ = session->GetSessionId();

    LOG_INFO("connection start session={}", session_id_);

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
        // 优化：将读回调也绑定到 strand
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
                                                   // 所有的业务分发现在都在 strand 中安全执行
                                                   HandlePacket(msgId, data, len);
                                               });

                                           DoRead();
                                       }
                                       else
                                       {
                                           LOG_WARN("client disconnected conn={}", connection_id_);
                                           Close(); // Close 内部有原子锁，是安全的
                                       }
                                   }));
}

void Connection::SendPacket(const Packet &packet)
{
    if (closed_)
        return;

    auto data = std::make_shared<std::vector<char>>(packet.Serialize());

    // 使用 boost::asio::post 将任务投递给 strand
    // 这样无论哪个线程（Actor 或 IO）调用 SendPacket，后续逻辑都在 strand 序列化执行
    boost::asio::post(strand_, [this, self = shared_from_this(), data]()
                      {
        if (write_queue_.size() > 1024) { 
            LOG_ERROR("Write queue overflow for session {}, kicking client", session_id_);
            Close(); 
            return;
}
        bool write_in_progress = !write_queue_.empty();
        write_queue_.push_back(data);

        // 如果当前没有正在进行的 async_write，则启动它
        if (!write_in_progress) {
            DoWrite();
        } });
}

void Connection::DoWrite()
{
    // 进入此函数时，已经在 strand 的保护下，无需加锁
    if (write_queue_.empty() || closed_)
        return;

    auto data = write_queue_.front();

    // 关键优化：使用 boost::asio::bind_executor 将回调绑定到 strand
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(*data),
        boost::asio::bind_executor(strand_,
                                   [this, self = shared_from_this(), data](boost::system::error_code ec, std::size_t)
                                   {
                                       if (ec)
                                       {
                                           Close();
                                           return;
                                       }

                                       write_queue_.pop_front();

                                       if (!write_queue_.empty())
                                       {
                                           DoWrite();
                                       }
                                   }));
}

void Connection::Close()
{
    // 1. 原子检查，确保只关闭一次
    if (closed_.exchange(true))
        return;

    // 2. 将清理逻辑 post 到 strand，确保与正在进行的 DoWrite/DoRead 串行化
    // 这样可以保证在销毁 session 之前，所有队列里的数据都已经处理完毕或被放弃
    boost::asio::post(strand_, [this, self = shared_from_this()]()
                      {
        // 清理发送队列，释放内存
        write_queue_.clear();

        // 获取并停止 Actor
        auto session = SessionManager::Instance().GetSession(session_id_);
        if (session)
        {
            auto actor = session->GetActor();
            if (actor)
            {
                actor->Stop();
                LOG_INFO("Actor for session {} stopped.", session_id_);
            }
        }

        // 原有的清理流程
        SessionManager::Instance().RemoveSession(session_id_);
        ConnectionManager::Instance().RemoveConnection(connection_id_);

        if (socket_.is_open())
        {
            boost::system::error_code ec;
            socket_.close(ec);
        } });
}