#ifndef __SOCKET_SERVER_HPP__
#define __SOCKET_SERVER_HPP__

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <work_queue.hpp>
#include <thread_pool.hpp>
#include <order.hpp>
#include <conn_info.hpp>

namespace trading {

  namespace mti = boost::multi_index;

  //############################################################################
  /// CLASS: Socket Server
  //############################################################################
  class socket_server_t {
  public:

    //##########################################################################
    /// Initialize
    ///
    /// - Initialize thread pool to number of threads.
    /// - Create server socket.
    /// - Bind server socket.
    /// - Listen on server socket.
    ///
    /// @param[in] port         server port
    /// @param[in] nreaders     number of reader threads
    /// @param[in] nprocessors  number of processor threads
    /// @return                 none
    /// @throws                 std::string if any step fails
    //##########################################################################
    void init(uint16_t port, size_t nreaders, size_t nprocessors);

    //##########################################################################
    /// Run
    ///
    /// - Launch reader threads.
    /// - Launch processor threads.
    /// - In loop accept socket.
    /// - Put on client socket queue.
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        none
    /// @throws        std::string if any step fails
    //##########################################################################
    void run();

  private:

    //##########################################################################
    /// Reader Thread
    ///
    /// - Get next socket from socket queue.
    /// - Read trader id from socket.
    /// - Create conn info w/ trader id and socket.
    /// - Add conn info instance to conn info table.
    /// - In a loop:
    /// - Read from socket into order struct.
    /// - Zero byte read indicates client closed connection.
    /// - Remove entry from conn info table keyed by socket.
    /// - Break out of inner loop and get next socket connection.
    /// - For a good read, create order ptr from order structure.
    /// - Add order ptr to work queue.
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        none
    /// @throws        none
    //##########################################################################
    void reader_thread();

    //##########################################################################
    /// Processor Thread
    ///
    /// - Get next work item from work queue.
    /// - Use order manager to process the order.
    /// - For each processed order:
    /// - Get the conn info shared ptr from order
    /// - If the conn info shared ptr is null, the connection has been closed.
    /// - Otherwise respond to client using the socket in the conn info.
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        none
    /// @throws        none
    //##########################################################################
    void processor_thread();

    /// copyable mutex
    typedef boost::shared_ptr<boost::mutex> mutex_ptr;

    //#########################################################################
    /// Multi-index container tag names
    //#########################################################################
    struct TRADER_INDEX {};
    struct SOCKET_INDEX {};

    //#########################################################################
    /// Connection Info Table
    /// - TRADER_INDEX
    /// - SOCKET_INDEX
    //#########################################################################
    typedef mti::multi_index_container<
      conn_info_ptr,
      mti::indexed_by<
        mti::ordered_unique<
          mti::tag<TRADER_INDEX>,
          BOOST_MULTI_INDEX_MEMBER(conn_info_t, int, trader_id_)
        >,
        mti::ordered_unique<
          mti::tag<SOCKET_INDEX>,
          BOOST_MULTI_INDEX_MEMBER(conn_info_t, int, socket_)
        >
      >
    > conn_info_table_t;

    //##########################################################################
    /// Shortened index typedefs
    //##########################################################################
    typedef conn_info_table_t::index<TRADER_INDEX>::type  trader_index_t;
    typedef conn_info_table_t::index<SOCKET_INDEX>::type  socket_index_t;

    /// work queue of order pointers
    typedef concurrent::queue_t<order_ptr> work_queue_t;

    /// work queue of socket descriptors
    typedef concurrent::queue_t<int> sockets_t;

    int               socket_;           /// listening socket
    size_t            nreaders_;         /// number of readers
    size_t            nprocessors_;      /// number of processors
    sockets_t         sockets_;          /// client socket connections
    work_queue_t      work_queue_;       /// work item queue
    order_manager_t   order_manager_;    /// trade order manager
    conn_info_table_t conn_info_table_;  /// connection info table
    boost::mutex      mutex_;            /// sync mechanism
  };

}  /// namespace trading

#endif
