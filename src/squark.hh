#pragma once

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

#include "bucket.hh"

#define NAME                 "squark"
#define AUTHOR               "Diego Catalao <diegocatalao@ieee.org>"
#define VERSION              "0.1.0"
#define LICENSE              "MIT"
#define PACKAGE              "squark"
#define ORIGIN               "https://github.com/diegocatalao/squark"
#define CLASSIFICATION       "Filter/Effect/Video"
#define DESCRIPTION          "A image thumbnail splitter for video streaming"

#define DEFAULT_QUEUE_SIZE   20
#define DEFAULT_IMAGE_PREFIX "thumbnail"
#define DEFAULT_SLOTS        9
#define MAX_SLOTS            16
#define MIN_SLOTS            1

G_BEGIN_DECLS

#define GST_TYPE_SQUARK            (gst_squark_get_type())
#define GST_SQUARK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_SQUARK, GstSquark))
#define GST_SQUARK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_SQUARK, GstSquarkClass))
#define GST_IS_SQUARK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_SQUARK))
#define GST_IS_SQUARK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_SQUARK))

enum { PROP_0, PROP_PREFIX, PROP_SLOTS, N_PROPERTIES };

typedef struct _GstSquark {
  GstBaseTransform element;
  /** setup the opaque gst_squark data structure to keep some params */
  squark::Bucket* bucket;

  /** setup the opaque gst_squark data structure to receive params */
  gchar* prefix;
  guint  slots;
} GstSquark;

typedef struct _GstSquarkClass {
  GstBaseTransformClass parent_class;
} GstSquarkClass;

G_END_DECLS
