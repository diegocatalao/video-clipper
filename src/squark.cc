#include <gst/video/video.h>
#include <stdlib.h>
#include <atomic>
#include <chrono>

#include "bucket.hh"
#include "debug.hh"
#include "squark.hh"

G_DEFINE_TYPE(GstSquark, gst_squark, GST_TYPE_BASE_TRANSFORM);

/** 
 * 
 */
static GParamSpec* properties[N_PROPERTIES] = {nullptr};

using steady_clock_t = std::chrono::steady_clock;
using ns_t = std::chrono::nanoseconds;
static std::atomic<int64_t> last_ingest_time_ns(0);

static GstFlowReturn gst_squark_transform_ip(GstBaseTransform* base, GstBuffer* buffer) {
  GstSquark* squark = GST_SQUARK(base);

  GstVideoInfo info;
  GstCaps*     caps = gst_pad_get_current_caps(GST_BASE_TRANSFORM_SRC_PAD(base));

  if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
    if (caps)
      gst_caps_unref(caps);
    return GST_FLOW_OK;
  }

  if (!caps) {
    GST_ERROR("fail to get the current caps for this buffer, skipped");
    return GST_FLOW_ERROR;
  }

  if (!gst_video_info_from_caps(&info, caps)) {
    GST_ERROR("fail to get the video information for this buffer, skipped");
    gst_caps_unref(caps);
    return GST_FLOW_ERROR;
  }

  int w = GST_VIDEO_INFO_WIDTH(&info);
  int h = GST_VIDEO_INFO_HEIGHT(&info);

  if (w <= 0 || h <= 0) {
    GST_WARNING("The video dimensions are invalid for now, skipped");
    gst_caps_unref(caps);
    return GST_FLOW_ERROR;
  }

  if (!squark->bucket) {
    squark->bucket = new squark::Bucket(DEFAULT_QUEUE_SIZE, squark->prefix, squark->slots, w, h);
    squark->bucket->start_digest_thread();
  }

  auto now_ns = std::chrono::duration_cast<ns_t>(steady_clock_t::now().time_since_epoch()).count();
  int64_t last_ns = last_ingest_time_ns.load();

  if (last_ns == 0 || (now_ns - last_ns) >= 8000000000LL) {
    try {
      GstBuffer* copy = gst_buffer_copy(buffer);
      squark->bucket->ingest(copy);
      last_ingest_time_ns.store(now_ns);
    } catch (const std::exception& exception) {
      GST_ERROR("fail to ingest the buffer: '%s'", exception.what());
    } catch (...) {
      GST_ERROR("an unknown exception occurred when trying to ingest the buffer");
    }
  }

  gst_caps_unref(caps);
  return GST_FLOW_OK;
}

static void gst_squark_set_property(GObject* object, guint id, const GValue* value,
                                    GParamSpec* spec) {
  GstSquark* squark = GST_SQUARK(object);

  switch (id) {
    case PROP_PREFIX:
      g_free(squark->prefix);
      squark->prefix = g_value_dup_string(value);
      break;
    case PROP_SLOTS:
      squark->slots = g_value_get_uint(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, spec);
  }
}

static void gst_squark_get_property(GObject* object, guint id, GValue* value, GParamSpec* spec) {
  GstSquark* squark = GST_SQUARK(object);

  switch (id) {
    case PROP_PREFIX:
      g_value_set_string(value, squark->prefix);
      break;
    case PROP_SLOTS:
      g_value_set_uint(value, squark->slots);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, spec);
  }
}

static void gst_squark_class_init(GstSquarkClass* klass) {
  GstElementClass*       element_class = GST_ELEMENT_CLASS(klass);
  GstBaseTransformClass* base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

  base_transform_class->transform_ip = gst_squark_transform_ip;

  gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass), NAME, CLASSIFICATION, DESCRIPTION,
                                        AUTHOR);

  static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw,format=RGB"));

  static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw,format=RGB"));

  gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&sink_template));
  gst_element_class_add_pad_template(element_class, gst_static_pad_template_get(&src_template));

  /** set the all properties to this plugin */
  properties[PROP_PREFIX] =
    g_param_spec_string("prefix", "Image prefix", "The image file prefix (e.g.: <prefix>-1.img",
                        DEFAULT_IMAGE_PREFIX, G_PARAM_READWRITE);

  properties[PROP_SLOTS] = g_param_spec_uint(
    "slots", "Slots of a single image", "The image slots quantity in a single image", MIN_SLOTS,
    MAX_SLOTS, DEFAULT_SLOTS, G_PARAM_READWRITE);

  GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->set_property = gst_squark_set_property;
  gobject_class->get_property = gst_squark_get_property;

  for (guint i = 1; i < N_PROPERTIES; i++)
    g_object_class_install_property(gobject_class, i, properties[i]);
}

static void gst_squark_init(GstSquark* squark) {
  squark::debug::initialize_debug();

  squark->bucket = nullptr;
  squark->slots = DEFAULT_SLOTS;
  squark->prefix = g_strdup(DEFAULT_IMAGE_PREFIX);
}

static gboolean plugin_init(GstPlugin* plugin) {
  return gst_element_register(plugin, "squark", GST_RANK_NONE, GST_TYPE_SQUARK);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, squark, DESCRIPTION, plugin_init, VERSION,
                  LICENSE, PACKAGE, ORIGIN)
