#include "debug.hh"

GST_DEBUG_CATEGORY(gst_squark_debug);

namespace squark {
  namespace debug {
    void initialize_debug() {
      GST_DEBUG_CATEGORY_INIT(gst_squark_debug, "squark", 0, "Debug messages for Squark plugin");
    }
  }  // namespace debug
};  // namespace squark
