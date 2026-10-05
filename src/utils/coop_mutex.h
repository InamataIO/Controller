#pragma once

namespace inamata {
namespace utils {

struct CoopMutex {
  bool locked = false;

  bool tryLock();

  void unlock();
};

}  // namespace utils
}  // namespace inamata