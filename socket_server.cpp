#include <arpa/inet.h>
#include <socket_server.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <xmit_order.hpp>
#include <tracer.hpp>

namespace trading {

  //############################################################################
  /// Initialize
  //############################################################################
  void
  socket_server_t::
  init(uint16_t port,
       size_t nreaders,
       size_t nprocessors) {

    /// create thread pool for # of readers and processors
    nreaders_ = nreaders;
    nprocessors_ = nprocessors;
    concurrent::thread_pool_t::instance().expand(nreaders + nprocessors);

    /// create server socket
    socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == -1) {
      perror(NULL);
      std::string s = "Socket creation failed: ";
      s += ::strerror(errno);
      throw s;
    }
    /// bind to socket
    struct sockaddr_in addr;
    bzero((char *) &addr, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(socket_, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
      perror(NULL);
      std::string s = "Socket bind failed: " + std::string(::strerror(errno));
      throw s;
    }
    /// listen on socket
    ::listen(socket_, 5);
  }

  //############################################################################
  /// Run
  //############################################################################
  void
  socket_server_t::
  run() {

    /// launch socket reader threads
    concurrent::thread_pool_t& pool = concurrent::thread_pool_t::instance();
    for (size_t i = 0; i < nreaders_; ++i) {
      pool.post(boost::bind(&socket_server_t::reader_thread, this));
    }
    /// launch order processor threads
    for (size_t i = 0; i < nprocessors_; ++i) {
      pool.post(boost::bind(&socket_server_t::processor_thread, this));
    }
    /// in loop start accepting client connections
    while (true) {

      socklen_t len = 0;
      struct sockaddr_in addr;
      bzero((char *) &addr, sizeof(addr));

      int socket = ::accept(socket_, (struct sockaddr *) &addr, &len);
      TRACE_BEGIN << "client connected socket: " << socket << std::endl;
      TRACE_END
      if (socket == -1) {
        perror(NULL);
        std::string s = "Socket accept failed: ";
        s += ::strerror(errno);
        throw s;
      }
      /// reader thread will pick up this socket connection
      sockets_.push(socket);
    }
  }
 
  //############################################################################
  /// Reader Thread
  //############################################################################
  void
  socket_server_t::
  reader_thread() {

    while (true) {

      /// get next socket from sockets queue
      int socket = sockets_.pop_front();

      /// read the connecting client's trader id
      TRACE_BEGIN << "waiting to read trader_id" << std::endl; TRACE_END

      char trader_id_buf[8] = {0};
      ssize_t n = ::read(socket, &trader_id_buf, 8);
      if (n != sizeof(trader_id_buf)) {
        TRACE_BEGIN << "Bad protocol, expected trader_id after connect"
                    << std::endl; TRACE_END
        continue;
      }
      int trader_id = ::atoi(trader_id_buf);
      TRACE_BEGIN << "received trader id: " << trader_id
                  << std::endl; TRACE_END

      ////////
      /// connection complete - create conn_info with socket and trader id
      /// and it to the conn_info_table.
      ////////
      conn_info_ptr cip = boost::make_shared<conn_info_t>();
      cip->socket_ = socket;
      cip->trader_id_ = trader_id;
      {
        boost::lock_guard<boost::mutex>  lock(mutex_);
        conn_info_table_.insert(cip);
      }
      for (;;) {

        /// read from socket into order tmp strucuture
        transmission::order_t ord;
        ssize_t n = ::read(socket, &ord, sizeof(ord));

        ////////
        /// if zero bytes read, close socket since client is done
        /// remove entry from connection info table; if a pending
        /// is processed after this, then the processing thread
        /// won't attempt to send a message on a closed socket
        /// stop the read loop and get the next socket to handle
        ////////
        if (n == 0) {
          TRACE_BEGIN << "client closed connection on socket: "
                      << socket << std::endl; TRACE_END
          boost::lock_guard<boost::mutex>  lock(mutex_);
          conn_info_table_.erase(cip->socket_);
          ::close(socket);
          break;
        }
        /// partial read
        else if (n != sizeof(ord)) {
          TRACE_BEGIN << "Bad protocol client writes don't appear to be "
                     << "of fixed size. Read: " << n << ", should be: "
                     << sizeof(ord) << std::endl; TRACE_END
          break;
        }
        TRACE_BEGIN << "received order: " << ord << std::endl; TRACE_END

        /// read full data, create the real order
        order_t::side_t side = ord.side_ == 0 ? order_t::buy : order_t::sell;
        order_ptr order = boost::make_shared<order_t>(
          ord.stock_, ord.trader_, ord.trader_id_, ord.quantity_, side, cip);

        /// add to the work queue
        work_queue_.push(order);
      }
    }
  }

  //############################################################################
  /// Processor Thread
  //############################################################################
  void
  socket_server_t::
  processor_thread() {

    while (true) {

      /// pop next order from front of work queue
      order_ptr order = work_queue_.pop_front();

      /// give order to order manager to process
      orders_t to_notify;
      {
        boost::lock_guard<boost::mutex> lock(mutex_);
        order_manager_.process_order(order, to_notify);
      }
      /// for each affected order, notify client
      for (size_t i = 0; i < to_notify.size(); ++i) {

        ////////
        /// get shared ptr to conn_info's weak_ptr; if shared ptr doesn't
        /// exist, socket was closed by client
        ////////
        order_ptr& order = to_notify[i];
        conn_info_ptr conn = order->conn_info();
        if (! conn) {
          TRACE_BEGIN << "cannot respond to client - socket has been closed. "
                      << "order: " << order << std::endl; TRACE_END
          continue;
        }
        /// create order tmp for transmission
        transmission::order_t ord(order);

        /// write thread-safe to client socket 
        boost::lock_guard<boost::mutex> lock(conn->mutex_);
        ssize_t n = ::write(conn->socket_, &ord, sizeof(ord));
        if (n != sizeof(ord)) {
          TRACE_BEGIN << "write to socket failed. wrote: " << n << " vs "
                      << sizeof(ord) << " for order: " << order
                      << std::endl; TRACE_END
        }
      }
    }
  }

}  /// namespace trading

//##############################################################################
/// Main
//##############################################################################
int main(int argc, const char** argv) {

  if (argc != 4) {
    std::cout << "Usage: <" << argv[0] << "> <server port> "
              << "<# of reader threads> <# of processor threads>"
              << std::endl;
    return -1;
  }
  uint16_t port = ::atoi(argv[1]);
  int nreaders = ::atoi(argv[2]);
  int nprocessors = ::atoi(argv[3]);

  trading::socket_server_t server;
  try {
    /// trading::tracer_t::instance().disable();
    server.init(port, nreaders, nprocessors);
    server.run();
  } 
  catch (const std::string& ex) {
    std::cerr << "Socket server caught: " << ex << std::endl;
  }
}
