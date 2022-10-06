#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__

#include <string>
#include <boost/thread.hpp>

namespace trading {

  //############################################################################
  /// CLASS: Socket Client
  //############################################################################
  class client_t {
  public:

    //##########################################################################
    /// Initialize
    ///
    /// - Initializes members from parameters.
    /// - Creates and expands thread pool.
    /// - Launches a separate thread for each sender.
    ///
    /// @param[in] host         server port
    /// @param[in] port         server port
    /// @param[in] senders      number of reader threads
    /// @param[in] nprocessors  number of total orders
    /// @return                 none
    /// @throws                 std::string if any step fails
    //##########################################################################
    void init(const std::string& host,
              const size_t port,
              const size_t nsenders,
              const size_t norders);
  private:

    //##########################################################################
    /// Sender Thread
    ///
    /// - Creates stream socket.
    /// - Connects to socket server.
    /// - Sends client/trader id to server
    /// - Launches receiver thread.
    /// - Sends nbatch_size_ orders to socket server.
    ///
    /// @param[in] trader_id  some artificial client/trader id
    /// @return               none
    /// @throws               none
    //##########################################################################
    void sender(int trader_id);

    //##########################################################################
    /// Receiver Thread
    ///
    /// - Reads order response from socket server
    /// - Trades out order response.
    ///
    /// @param[in] socket  socket connection to server
    /// @return            none
    /// @throws            none
    //##########################################################################
    void receiver(int socket);

    std::string   host_;
    size_t        port_;
    size_t        nsenders_;
    size_t        norders_;
    size_t        nbatch_size_;
    boost::mutex  mutex_;
  };

}  /// namespace trading

#endif
