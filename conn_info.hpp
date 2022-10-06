#ifndef __CONNECTION_INFO_HPP__
#define __CONNECTION_INFO_HPP__

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace trading {

  //############################################################################
  /// STRUCT: Connection Info
  ///
  /// Identifies a socket connection from a client/trader. Contains trader id
  /// and socket fd; order_t contains a weak ptr to an instance of conn info
  /// in the conn info table (see socket_server_t). If the client disconnects,
  /// the conn info entry is removed from the table. The shared_ptr obtained
  /// from the weak ptr can then be used to check if the connection has been
  /// closed.
  //############################################################################
  struct conn_info_t {
    int           trader_id_;
    int           socket_;
    boost::mutex  mutex_; 
  };
  typedef boost::shared_ptr<conn_info_t> conn_info_ptr;
  typedef boost::weak_ptr<conn_info_t>   conn_info_wptr;

}

#endif
