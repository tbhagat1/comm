#include <order.hpp>
#include <vector>
#include <fstream>

void split(const std::string& s,
           std::vector<std::string>& v) {

  for (size_t i = 0; i < s.size(); ++i) {
    if (::isspace(s[i])) continue;
    std::string tmp;
    while (!::isspace(s[i]) && i < s.size()) {
     tmp += s[i++];
    }
    v.push_back(tmp);
  }
}

int main(int argc, const char** argv) {

  if (argc != 2) {
    std::cout << "Usage: "
              << argv[0]
              << " <input test file>"
              << std::endl;
    return -1;
  }
  std::ifstream ifs(argv[1]);
  if (! ifs) {
    std::cout << "Failed to open: " << argv[1] << std::endl;
    return -1;
  }
  trading::order_manager_t om;
  std::string line;

  while (std::getline(ifs, line)) {
    std::vector<std::string> v;
    split(line, v);
    int qty = ::atoi(v[2].c_str());
    trading::order_t::side_t side =
      static_cast<trading::order_t::side_t>(::atoi(v[3].c_str()));
    trading::order_ptr order =
        boost::make_shared<trading::order_t>(
          v[0], v[1], qty, side);
    om.process_order(order);
    std::cout << om << std::endl;
  }
}
