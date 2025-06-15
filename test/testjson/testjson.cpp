#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include <iostream>
#include <map>
#include <string>
#include <vector>
void func1() {
  json js;
  js["msg_type"] = 2;
  js["from"] = "zhang san";
  js["to"] = "lisi";
  std::string send_buf = js.dump();
  std::cout << "反序列化 : "
            << "\n";
  auto r_js = json::parse(send_buf);
  std::cout << r_js << "\n";
  std::cout << r_js["msg_type"] << "\n";
}

void func2() {
  json js;
  js["msg_type"] = 2;
  js["id"] = {1, 2, 3, 4, 5};
  js["from"] = "zhang san";
  js["to"] = "lisi";
  js["msg"]["zhang san"] = "hello world";
  std::cout << js << "\n";
}

//容器序列化
void func3() {
  json js;
  std::vector<int> vec{1, 2, 3};
  js["list"] = vec;
  std::map<int, std::string> m = {{1, "a"}, {2, "b"}};
  js["path"] = m;
  std::cout << js << "\n";
}

int main() {
  func1();
  func2();
  func3();
}