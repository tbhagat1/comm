#ifndef __XMIT_ORDER_HPP__
#define __XMIT_ORDER_HPP__

#include <iostream>
#include <order.hpp>

namespace transmission {

  //############################################################################
  /// STRUCT: Transmission Order
  //############################################################################
  struct order_t {

    //##########################################################################
    /// Default Constructor
    ///
    /// @param[in]     none
    /// @param[inout]  none
    /// @return        none
    /// @throws        none
    //##########################################################################
    order_t() :
      trader_id_(0),
      quantity_(0),
      balance_(0),
      side_(0) {
      ::memset(&stock_,  '\0', sizeof(stock_));
      ::memset(&trader_, '\0', sizeof(trader_));
    }

    //##########################################################################
    /// Constructor (from order_ptr)
    ///
    /// @param[in]     order  order pointer
    /// @param[inout]         none
    /// @return               none
    /// @throws               none
    //##########################################################################
    order_t(const trading::order_ptr& order) :
      trader_id_(order->trader_id()),
      quantity_(order->quantity()),
      balance_(order->balance()),
      side_(order->side()) {
      strcpy(stock_, order->stock().c_str());
      strcpy(trader_, order->trader().c_str());
    }

    char stock_[8];
    char trader_[64];
    int  trader_id_;
    int  quantity_;
    int  balance_;
    int  side_;
  };

  //############################################################################
  /// Opeartor<<
  ///
  /// @param[inout]  os    output stream
  /// @param[in]    order  order pointer
  /// @return              updated output stream
  /// @throws              none
  //############################################################################
  std::ostream& operator<<(std::ostream& os, const order_t& order) {
    os << order.stock_     << "  "
       << order.trader_    << "  "
       << order.trader_id_ << "  "
       << order.quantity_  << "  "
       << order.balance_  << "  "
       << order.side_     << "("
       << (order.side_ == 0 ? "Buy" : "Sell") << ")";
    return os;
  }

}  /// namespace trading

#endif
