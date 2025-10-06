#pragma once

#include <gst/gst.h>

GST_DEBUG_CATEGORY_EXTERN(gst_squark_debug);
#define GST_CAT_DEFAULT gst_squark_debug

namespace squark {
  namespace debug {
    void initialize_debug();
  };
};  // namespace squark
