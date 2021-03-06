#include <iostream>
#include <zmq.hpp>
#include <json.hpp>

using json = nlohmann::json;

/* use to and from JSON
 * push in one, and pop in the other
 * use single thread
 */
/*
 * what needs to be done in the other thread
 * serialize, remove from the map
 */

struct tick {
  tick() : p1(0.0), p2(0.0),
  p1_set(0), p2_set(0) {}
  float p1, p2;
  bool p1_set, p2_set;
#if defined DEBUG
  long long int t1, t2;
#endif
};


std::map<long long int, tick> tickMap;
int main() {
  // need a shared map
  std::shared_ptr<zmq::context_t> context = std::make_shared<zmq::context_t>(1);
  zmq::socket_t receiver(*context.get(), ZMQ_PULL);
#if defined ALIGN_PORT
  std::string bind_addr = "tcp://*:" + std::to_string(ALIGN_PORT);
  receiver.bind(bind_addr.c_str());
#elif
  std::cout << "ERROR: ALIGN_PORT NOT DEFINED" << std::endl;
  return 1;
#endif

  zmq::socket_t sender(*context.get(), ZMQ_PUSH);
#if defined STRATEGY_PORT
  std::string send_addr = "tcp://*:"
    + std::to_string(STRATEGY_PORT);
  sender.bind(send_addr.c_str());
#endif

  zmq::message_t message;

// receive message
// decode json

  while (true) {
      receiver.recv(&message);
      std::string smessage(static_cast<char*>(message.data()), message.size());
      // convert to json string
      json jmsg = json::parse(smessage);
      long long int ts = jmsg["T"];

      tickMap.emplace(ts, tick());

      if (jmsg["id"]  == 1) {
        tickMap[ts].p1 = jmsg["p"];
        tickMap[ts].p1_set = true;
#if defined DEBUG
        tickMap[ts].t1 = jmsg["t"];
#endif
      }
      else {
        tickMap[ts].p2 = jmsg["p"];
        tickMap[ts].p2_set = true;
#if defined DEBUG
        tickMap[ts].t2 = jmsg["t"];
#endif
      }

      if(tickMap[ts].p1_set && tickMap[ts].p2_set) {
        auto it = tickMap.find(ts);
// #if defined DEBUG
//         long long int get_ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//         std::cout << get_ts - std::max(it->second.t1, it->second.t2) << std::endl;
// #endif
        //TODO send
        {
          json j;
          j["T"] = ts;
          j["binance"] = it->second.p1;
          j["bitmex"] = it->second.p2;
#if defined DEBUG
          j["bin_delay"] = ts-it->second.t1;
          j["bit_delay"] = ts-it->second.t2;
#endif

          std::string _msg = j.dump();
          int len = _msg.length();
          zmq::message_t msg (len);
          memcpy(msg.data(), _msg.c_str(), len);
          sender.send(msg);
        }
        tickMap.erase(it);
      }
  }
  return 0;
}
