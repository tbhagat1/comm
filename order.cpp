#include <vector>
#include <order.hpp>
#include <tracer.hpp>
#include <sstream>
#include <boost/thread.hpp>

namespace trading {

  //###########################################################################
  /// Process Order
  //###########################################################################
  void
  order_manager_t::
  process_order(order_ptr& order,
                orders_t& to_notify) {

    TRACE_BEGIN << "processing order: " << std::endl
      << "*****************************************************************\n"
      << order << std::endl
      << "*****************************************************************\n";
    TRACE_END

    /// reverse the side to fill the order
    order_t::side_t other_side =
      order->side() == order_t::buy ? order_t::sell : order_t::buy;

    /// get the stock side index and find by stock and side
    stock_side_index_t& ssi = orders_.get<STOCK_SIDE_INDEX>();
    ssi_index_iterator_pair_t i =
      ssi.equal_range(boost::make_tuple(order->stock(), other_side));

    /// if order doesn't exist, add it and return
    if (i.first == i.second) {
      ssi.insert(order);
      TRACE_BEGIN << "inserted: " << order << std::endl
                  << *this << std::endl;
      TRACE_END
      return;
    }
    /// if the input order isn't complete it must be added to the order table
    bool must_add = false;

    /// attempt to fill the order with existing orders
    while (i.first != i.second) {

      /// subtract traded balance from both sides of the trade
      order_ptr rhs = *i.first;

      int rhs_bal = rhs->balance() - order->balance();
      int lhs_bal = order->balance() - rhs->balance();
      
      ////////
      /// if the balance on the order in the order table falls to or
      /// below zero, clear the balance on the order, add it to the
      /// notification list and remove it from the order table
      ////////
      if (rhs_bal <= 0) {
        rhs->balance(0);
        to_notify.push_back(rhs);
        i.first = ssi.erase(i.first);
      }
      /// otherwise update the balance and proceed to the next order
      else {
        rhs->balance(rhs_bal);
        ++i.first;
      }
      ////////
      /// if the balance on the input order falls to or below zero,
      /// clear the balance on the order and remember not to add this
      /// order to the order table since it is complete
      ////////
      if (lhs_bal <= 0) {
        order->balance(0);
        to_notify.push_back(order);
        must_add = false;
        break;
      }
      /// otherwise the order hasn't been completed yet, remember to add it
      else {
        order->balance(lhs_bal);
        must_add = true;
      }
    }
    if (must_add) {
      orders_.insert(order);
    }
    notify(to_notify);
  }

  //###########################################################################
  /// Notify
  //###########################################################################
  void
  order_manager_t::
  notify(const orders_t& orders) {

    TRACE_BEGIN << *this << std::endl
      << "*****************************************************************\n"
      << "Updated orders: " << std::endl
      << "*****************************************************************\n";

    for (size_t i = 0; i < orders.size(); ++i)
      TRACE << orders[i] << std::endl;

    TRACE_END
  }

  //###########################################################################
  /// Operator<< (order_ptr)
  //###########################################################################
  std::ostream& operator<<(std::ostream& os, const order_ptr& order) {

    const char* side = order->side() == order_t::buy ? "Buy " : "Sell";
    os << order->stock()    << "\t"
       << order->quantity() << "\t"
       << order->balance()  << "\t"
       << side              << "\t"
       << order->trader();
    return os;
  }

  //###########################################################################
  /// Operator<< (order_manager_t)
  //###########################################################################
  std::ostream& operator<<(std::ostream& os, const order_manager_t& om) {

    os << "Order Table: " << std::endl;
    os << "*****************************************************************"
       << std::endl;
    const stock_index_t& stock_index = om.orders_.get<STOCK_INDEX>();
    for (stock_index_t::const_iterator i = stock_index.begin();
         i != stock_index.end();
         ++i) {
      const order_ptr& p = *i;
      os << p << std::endl;
    }
    os << "*****************************************************************";
    return os;
  }
}
