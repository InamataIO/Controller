#include "limit_event.h"

namespace inamata {
namespace utils {

const __FlashStringHelper* LimitEvent::event_type_key_ = FPSTR("event");
const __FlashStringHelper* LimitEvent::start_type_ = FPSTR("str");
const __FlashStringHelper* LimitEvent::continue_type_ = FPSTR("con");
const __FlashStringHelper* LimitEvent::end_type_ = FPSTR("end");

}  // namespace utils
}  // namespace inamata