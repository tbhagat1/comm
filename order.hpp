#ifndef __ORDER_HPP__
#define __ORDER_HPP__

#include <map>
#include <set>
#include <string>
#include <iostream>
#include <conn_info.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/thread.hpp>

namespace trading {

  namespace mti = boost::multi_index;

  class order_t;
  typedef boost::shared_ptr<order_t> order_ptr;
  typedef std::vector<order_ptr>     orders_t;

  //###########################################################################
  /// CLASS: Order
  ///
  /// Minimal trade order, contains:
  /// - Stock
  /// - Trader
  /// - Trader Id
  /// - Quantity (original order amount)
  /// - Balance (amount remaining on the order)
  /// - Side (buy or sell)
  /// - Connection Info Weak Ptr
  ///
  /// Shared pointers to orders are inserted into a multi-index container.
  //###########################################################################
  class order_t {
  public:

    /// Side - Buy or Sell
    enum side_t {
      buy,
      sell
    };

    //##########################################################################
    /// Constructor (Default)
    ///
    /// Initializes members to their defaults.
    ///
    /// @param[inout]  none
    /// @return        none
    /// @throws        none
    //##########################################################################
    order_t() :
      quantity_(0),
      balance_(0),
      side_(buy),
      trader_id_(0)
    {}

    //##########################################################################
    /// Constructor
    ///
    /// Initializes members from arguments.
    ///
    /// @param[in]  stock     stock name
    /// @param[in]  trader    trader name
    /// @param[in]  quantity  traded quantity
    /// @param[in]  side      side (buy or sell)
    /// @return               none
    /// @throws               none
    //##########################################################################
    order_t(const std::string& stock,
            const std::string& trader,
            int trader_id,
            int quantity,
            side_t side,
            const conn_info_ptr& conn_info) :
      stock_(stock),
      trader_(trader),
      trader_id_(trader_id),
      quantity_(quantity),
      balance_(quantity),
      side_(side),
      conn_info_(conn_info)
    {}

    //##########################################################################
    /// Stock Accessor
    ///
    /// @param[inout]  none
    /// @param[in]     none
    /// @return        stock name
    /// @throws        none
    //##########################################################################
    const std::string& stock() const { return stock_; }

    //##########################################################################
    /// Trader Accessor
    ///
    /// @param[inout]  none
    /// @param[in]     none
    /// @return        trader name
    /// @throws        none
    //##########################################################################
    const std::string& trader() const { return trader_; }

    //##########################################################################
    /// Trader Id Accessor
    ///
    /// @param[inout]  none
    /// @param[in]     none
    /// @return        quantity
    /// @throws        none
    //##########################################################################
    const int trader_id() const { return trader_id_; }

    //##########################################################################
    /// Quantity Accessor
    ///
    /// @param[inout]  none
    /// @param[in]     none
    /// @return        quantity
    /// @throws        none
    //##########################################################################
    const int quantity() const { return quantity_; }

    //##########################################################################
    /// Balance Accessor
    ///
    /// @param[inout]  none
    /// @param[in]     none
    /// @return        balance
    /// @throws        none
    //##########################################################################
    const int balance() const { return balance_; }

    //##########################################################################
    /// Side Accessor
    ///
    /// @param[inout]  none
    /// @param[in]     none
    /// @return        side
    /// @throws        none
    //##########################################################################
    const side_t side() const { return side_; }

    //##########################################################################
    /// Connection Info Accessor
    ///
    /// @param[inout]  none
    /// @param[in]     none
    /// @return        conn_info_t shared ptr obtained from member weak ptr
    /// @throws        none
    //##########################################################################
    conn_info_ptr conn_info() { return conn_info_.lock(); }

    //##########################################################################
    /// Stock Mutator
    ///
    /// @param[inout]  none
    /// @param[in]     stock
    /// @return        none
    /// @throws        none
    //##########################################################################
    void stock(const std::string& stock) { stock_ = stock; }

    //##########################################################################
    /// Trader Mutator
    ///
    /// @param[inout]  none
    /// @param[in]     trader
    /// @return        none
    /// @throws        none
    //##########################################################################
    void trader(const std::string& trader) { trader_ = trader; }

    //##########################################################################
    /// Quantity Mutator
    ///
    /// @param[inout]  none
    /// @param[in]     quantity
    /// @return        none
    /// @throws        none
    //##########################################################################
    void quantity(int quantity) { quantity_  = quantity; }

    //##########################################################################
    /// Balance Mutator
    ///
    /// @param[inout]  none
    /// @param[in]     balance
    /// @return        none
    /// @throws        none
    //##########################################################################
    void balance(int balance) { balance_ = balance; }

    //##########################################################################
    /// Side Mutator
    ///
    /// @param[inout]  none
    /// @param[in]     side
    /// @return        none
    /// @throws        none
    //##########################################################################
    void side(const side_t side) { side_ = side; }

  private:

    //##########################################################################
    /// Operator<< (order_ptr)
    ///
    /// Prints the output to the output stream.
    ///
    /// @param[inout]  os     output stream
    /// @param[in]     order  order pointer
    /// @return               updated output stream
    /// @throws               none
    //##########################################################################
    friend std::ostream& operator<<(std::ostream& os, const order_ptr& order);
    friend struct balance_updater_t;

    std::string     stock_;
    std::string     trader_;
    int             trader_id_;
    int             quantity_;
    int             balance_;
    side_t          side_;
    conn_info_wptr  conn_info_;
  };

  //############################################################################
  /// STRUCT: Balance Updater
  //############################################################################
  struct balance_updater_t {

    balance_updater_t(int balance) :
      balance_(balance)
    {}

    void operator()(order_ptr& order) {
      order->balance_ = balance_;
    }
    int balance_;
  };

  //############################################################################
  /// Multi-index container tag names
  //############################################################################
  struct STOCK_SIDE_INDEX {};
  struct STOCK_INDEX      {};
  struct TRADER_INDEX     {};

  //############################################################################
  /// Order table
  ///
  /// Indexed by:
  /// - STOCK_SIDE_INDEX - composite key comprised of stock and side (buy|sell)
  /// - STOCK_INDEX      - stock index  keyed by stock  name
  /// - TRADER_INDEX     - trader index keyed by trader name
  //############################################################################
  typedef mti::multi_index_container<
    order_ptr,
    mti::indexed_by<
      mti::ordered_non_unique<
        mti::tag<STOCK_SIDE_INDEX>,
        mti::composite_key<
          order_ptr,
          mti::const_mem_fun<
            order_t,
            const std::string&,
            &order_t::stock
          >,
          mti::const_mem_fun<
            order_t,
            const order_t::side_t,
            &order_t::side
          >
        >
      >,
      mti::ordered_non_unique<
        mti::tag<STOCK_INDEX>,
        mti::const_mem_fun<
          order_t,
          const std::string&,
          &order_t::stock
        >
      >,
      mti::ordered_non_unique<
        mti::tag<TRADER_INDEX>,
        mti::const_mem_fun<
          order_t,
          const std::string&,
          &order_t::trader
        >
      >
    >
  > order_table_t;

  //############################################################################
  /// Shortened index typedefs
  //############################################################################
  typedef order_table_t::index<STOCK_SIDE_INDEX>::type  stock_side_index_t;
  typedef order_table_t::index<STOCK_INDEX>::type       stock_index_t;
  typedef order_table_t::index<TRADER_INDEX>::type      trader_index_t;
  typedef std::pair<stock_side_index_t::iterator,
                    stock_side_index_t::iterator> ssi_index_iterator_pair_t;

  //############################################################################
  /// CLASS: Order Manager
  ///
  /// Rudimentary order manager; contains the order table and has an interface
  /// method to accept and process an order.
  //############################################################################
  class order_manager_t {
  public:

    //##########################################################################
    /// Process Order
    ///
    /// - Reverse the side on the input order.
    /// - Locate the [stock, side] tuple in the order table.
    /// - If not found, add the input order to the order table.
    /// - Otherwise iterate over all open orders in an attempt to complete
    ///   the input order.
    /// - If the balance on an order in the table goes to zero, remove it
    ///   from the order table and add to a notification list.
    /// - If the balance on the input order goes to zero, add it to the
    ///   notification list and stop the iteration.
    /// - If the input order was not filled after the iteration, add it to
    ///   the order table.
    /// - Notify any updated orders (print to screen).
    ///
    /// @param[inout]  none
    /// @param[in]     trader
    /// @return        none
    /// @throws        none
    //##########################################################################
    void process_order(order_ptr& order, orders_t& to_notify);

  private:

    //##########################################################################
    /// Notify
    ///
    /// Just print the list of updated orders. @todo: in a full application
    /// must send messages to the client program.
    ///
    /// @param[inout]  none
    /// @param[in]     trader
    /// @return        none
    /// @throws        none
    //##########################################################################
    void notify(const orders_t& orders);

    //##########################################################################
    /// Operator<< (order_manager_t)
    ///
    /// @param[inout]  none
    /// @param[in]     trader
    /// @return        none
    /// @throws        none
    //##########################################################################
    friend std::ostream& operator<<(std::ostream& os, const order_manager_t& om);

    order_table_t orders_;
    boost::mutex  mutex_;
  };

}  /// namespace trading

#endif
