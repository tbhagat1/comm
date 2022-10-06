#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread_pool.hpp>
#include <xmit_order.hpp>
#include <boost/bind.hpp>
#include <client.hpp>
#include <tracer.hpp>

namespace trading {

  //###########################################################################
  /// Initialize
  //###########################################################################
  void
  client_t::
  init(const std::string& host,
       const size_t port,
       const size_t nsenders,
       const size_t norders) {

    host_ = host;
    port_ = port;
    nsenders_ = nsenders;
    norders_ = norders;
    nbatch_size_ = norders_ / nsenders_;

    concurrent::thread_pool_t& pool = concurrent::thread_pool_t::instance();

    pool.expand(nsenders_*2);
    int trader_id = 100;

    for (size_t i = 0; i < nsenders_; ++i, ++trader_id) {
      pool.post(boost::bind(&client_t::sender, this, trader_id));
    }
  }

  //###########################################################################
  /// Some fake stocks
  //###########################################################################
  static const std::string stocks[] = {
    "IBM",
    "DEL",
    "SNY",
    "BBG",
    "MSN"
  };
  static const size_t nstocks = sizeof(stocks)/sizeof(std::string);

  //###########################################################################
  /// STRUCT: Trader Info
  //###########################################################################
  struct trader_info_t {
    trader_info_t(const std::string& name,
                  const int id) :
      name_(name),
      id_(id)
    {}
    std::string name_;
    int         id_;
  };

  //###########################################################################
  /// Some fake traders
  //###########################################################################
  static const trader_info_t trader_info[] = {
    trader_info_t("John",  100),
    trader_info_t("James", 101),
    trader_info_t("Fred",  102),
    trader_info_t("Tony",  103),
    trader_info_t("Mike",  104),
    trader_info_t("Jim",   105),
    trader_info_t("Dave",  106),
    trader_info_t("Andy",  107),
    trader_info_t("Dan",   108),
    trader_info_t("Luke",  109)
  };
  static const size_t ntraders = sizeof(trader_info)/sizeof(trader_info_t);

  //###########################################################################
  /// Sender Thread
  //###########################################################################
  void
  client_t::
  sender(int trader_id) {

    /// create socket
    int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    TRACE_BEGIN << "created socket: " << socket << std::endl; TRACE_END

    if (socket == -1) {
      TRACE_BEGIN << "Failed to create socket, errno: " << errno
                  << std::endl << " strerror: " << strerror(errno)
                  << std::endl; perror("Socket create: "); TRACE_END
      return;
    }
    /// access host entry for host name
    struct hostent* srv = gethostbyname(host_.c_str());
    if (! srv) {
      TRACE_BEGIN << "Failed to gethostbyname, errno: " << errno
                 << std::endl;
      TRACE << " strerror: " << strerror(errno) << std::endl;
      perror("gethostbyname:");
      TRACE_END
      return;
    }
    /// fill sockaddr_in structure, use the addr from gethostbyname
    struct sockaddr_in addr;
    ::bzero((char *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;
    ::bcopy((char *) srv->h_addr, (char *) &addr.sin_addr.s_addr, srv->h_length);
    addr.sin_port = htons(port_);
    
    /// connect to server
    if (connect(socket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
      TRACE_BEGIN << "Connect failed, errno: " << errno << std::endl
                  << " strerror: " << strerror(errno) << std::endl;
      perror("gethostbyname:");
      TRACE_END
      return;
    }
    /// write trader id post connect
    char trader_id_buf[8];
    sprintf(trader_id_buf, "%d", 100);
    ssize_t n = ::write(socket, &trader_id_buf, sizeof(trader_id_buf));
    if (n != sizeof(trader_id_buf)) {
      TRACE_BEGIN << "Send trader id failed, errno: " << errno << std::endl
                  << " strerror: " << strerror(errno) << std::endl;
      perror("gethostbyname:");
      TRACE_END
      return;
    }
    /// start receiver thread
    concurrent::thread_pool_t& pool = concurrent::thread_pool_t::instance();
    pool.post(boost::bind(&client_t::receiver, this, socket));

    size_t stock_ndx = 0;
    size_t trader_ndx = 0;
    int quantity = 100;
    int side_ndx = 0;

    for (size_t i = 0; i < nbatch_size_; ++i) {

      /// fill transmission order
      transmission::order_t order;

      strcpy(order.stock_, stocks[stock_ndx].c_str());
      strcpy(order.trader_, trader_info[trader_ndx].name_.c_str());
      order.trader_id_ = trader_info[trader_ndx].id_;
      order.quantity_ = quantity;
      order.balance_ = quantity;
      order.side_ = side_ndx;

      TRACE_BEGIN << "sending order: " << order << std::endl; TRACE_END

      /// write to socket which will send data to client
      ssize_t n = ::write(socket, &order, sizeof(order));
      if (n != sizeof(order)) {
        TRACE_BEGIN << "Failed to write order to socket. Bytes written: "
                    << n << " sizeof(order): 152" << std::endl; TRACE_END
        break;
      }
      /// move socket, trader and side indices
      stock_ndx = ++stock_ndx % nstocks;
      trader_ndx = ++trader_ndx % ntraders;
      side_ndx = ++side_ndx % 2;
      quantity += ++quantity % 100;
    }
  }

  //###########################################################################
  /// Receiver Thread
  //###########################################################################
  void
  client_t::
  receiver(int socket) {

    for (;;) {

      /// read order on socket
      transmission::order_t order;
      ssize_t n = ::read(socket, &order, sizeof(order));
      if (n != sizeof(order)) {
        TRACE_BEGIN << "Failed to read response from server. Bytes read: "
                    << n << " sizeof(order): " << sizeof(order) << std::endl;
        TRACE_END
        break;
      }
      TRACE_BEGIN << "received update on: " << order << std::endl; TRACE_END
    }
  }
}

int main(int argc, const char** argv) {
  if (argc != 5) {
    std::cout << "Usage: <" << argv[0] << "> <host> <port> "
              << "<# of sender threads> <# of sends>" << std::endl;
    return -1;
  }
  trading::client_t client;
  client.init(argv[1], ::atoi(argv[2]), ::atoi(argv[3]), ::atoi(argv[4]));
  concurrent::thread_pool_t::instance().iosvc().run();
  concurrent::thread_pool_t::instance().wait();
}
