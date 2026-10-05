#include "coop_mutex.h"

namespace inamata {
namespace utils {

bool CoopMutex::tryLock() {
  if (locked) {
    return false;
  }
  locked = true;
  return true;
}

void CoopMutex::unlock() { locked = false; }

}  // namespace utils
}  // namespace inamata