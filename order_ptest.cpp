#include <order.hpp>
#include <iostream>

static const std::string traders[] =
{
  "John",
  "Jim",
  "Rick",
  "Alan",
  "Mike",
  "Karl",
  "Fred",
  "Tim",
  "Andy",
  "Jack"
};
static const size_t ntraders = sizeof(traders)/sizeof(std::string);

static const std::string stocks[] =
{
  "DEL",
  "IBM",
  "SNY"
};
static const size_t nstocks = sizeof(stocks)/sizeof(std::string);

int main(int argc, const char** argv) {

  if (argc != 2) {
    std::cout << "Usage: "
              << argv[0]
              << " <iterations>"
              << std::endl;
    return -1;
  }
  size_t iters = ::atoi(argv[1]);
  if (iters > 10000000) {
    std::cout << "Iterations must between 1 and 10 million" << std::endl;
    return -1;
  }
  size_t base = 100;
  trading::order_manager_t om;
  for (size_t i = 0; i < iters; ++i) {
    size_t stock_ndx  = i % nstocks;
    size_t trader_ndx = i % ntraders;
    int qty = base +  i % 100;
    trading::order_t::side_t side = i % 2 == 0 ? trading::order_t::buy : trading::order_t::sell;
    trading::order_ptr order =
      boost::make_shared<trading::order_t>(
        stocks[stock_ndx], traders[trader_ndx], qty, side);
    om.process_order(order);
  }
}
