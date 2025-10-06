#include <gst/gst.h>
#include <gst/gstbuffer.h>
#include <stdlib.h>

#include "bucket.hh"
#include "debug.hh"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <thread>

namespace squark {

  Bucket::Bucket(int size, std::string prefix, int slots, int w, int h) {
    this->size = size;
    this->prefix = prefix;
    this->slots = slots;
    this->width = w;
    this->height = h;

    squark::debug::initialize_debug();
  }

  Bucket::Bucket(gint size, gchar* prefix, guint slots, gint w, gint h)
      : Bucket(static_cast<int>(size), std::string(prefix), static_cast<int>(slots),
               static_cast<int>(w), static_cast<int>(h)) {}

  void Bucket::calculate_mse_and_push(GstBuffer* buffer) {
    GstMapInfo                   map;
    std::unique_lock<std::mutex> lock(this->mutex);

    static const double tau = 0.8;

    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      GST_DEBUG("fail to map buffer PTS=%" GST_TIME_FORMAT, GST_TIME_ARGS(buffer->pts));
      gst_buffer_unref(buffer);
      return;
    }

    uint8_t* current = map.data;
    size_t   pixels = width * height * 3;
    double   current_mse = 0.0;

    if (this->preview == nullptr) {
      this->preview = current;
      this->last_pts = buffer->pts;
    }

    for (size_t i = 0; i < pixels; ++i) {
      double diff = static_cast<double>(current[i]) - this->preview[i];
      current_mse += diff * diff;
    }

    double dt = (buffer->pts - this->last_pts) / 1e9;
    double alpha = 1.0 - std::exp(-dt / tau);
    current_mse /= static_cast<double>(pixels);

    if (this->mse > 0 && current_mse > 1.7 * this->mse) {
      this->queue.push(buffer);
      GST_DEBUG("buffer was pushed PTS=%" GST_TIME_FORMAT, GST_TIME_ARGS(buffer->pts));
    } else {
      this->mse = (1.0 - alpha) * this->mse + alpha * current_mse;
    }

    this->mse = current_mse;
  }

  void Bucket::ingest(GstBuffer* buffer) {
    //GST_DEBUG("Ingesting buffer PTS=%" GST_TIME_FORMAT, GST_TIME_ARGS(buffer->pts));

    if (this->queue.size() >= this->size) {
      GST_WARNING("The queue is full %zu/%d", this->queue.size(), this->size);
      return;
    }

    this->queue.push(buffer);
  }

  void Bucket::start_digest_thread() {
    if (this->keep_alive) {
      GST_WARNING("This operation is already called and will skip it for now");
      return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    this->keep_alive = true;

    std::thread thread_digest(&Bucket::digest, this);
    thread_digest.detach();
  }

  void Bucket::shutdown() {
    std::lock_guard<std::mutex> lock(mutex);
    this->keep_alive = false;
  }

  void Bucket::digest() {
    std::vector<GstBuffer*> buffers;
    int                     index = 0;

    while (keep_alive) {
      {
        std::unique_lock<std::mutex> lock(this->mutex);
        if (this->queue.size() < static_cast<size_t>(this->slots)) {
          lock.unlock();
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }

        for (int i = 0; i < this->slots; i++) {
          buffers.push_back(this->queue.front());
          this->queue.pop();
        }
      }

      int cols = static_cast<int>(std::ceil(std::sqrt(buffers.size())));
      int rows = static_cast<int>(std::ceil(static_cast<float>(buffers.size()) / cols));

      int tile_width = width / cols;
      int tile_height = height / rows;

      cv::Mat canvas(height, width, CV_8UC3, cv::Scalar(0, 0, 0));

      for (int i = 0; i < buffers.size(); i++) {
        GstBuffer* buffer = buffers[i];
        GstMapInfo map;

        if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
          GST_DEBUG("fail to map buffer PTS=%" GST_TIME_FORMAT, GST_TIME_ARGS(buffer->pts));
          gst_buffer_unref(buffer);
          continue;
        }

        cv::Mat clip;
        cv::Mat frame(this->height, this->width, CV_8UC3, map.data);

        gst_buffer_unmap(buffer, &map);
        gst_buffer_unref(buffer);

        // ignore it and pass to another is the best for now
        if (frame.empty())
          continue;

        cv::resize(frame, clip, cv::Size(tile_width, tile_height));

        int row = i / cols;
        int col = i % cols;

        int x = col * tile_width;
        int y = row * tile_height;

        clip.copyTo(canvas(cv::Rect(x, y, tile_width, tile_height)));
      }

      if (!canvas.empty()) {
        std::stringstream str_stream;
        str_stream << this->prefix << "-" << (++index) << ".jpg";
        cv::imwrite(str_stream.str(), canvas);
      }

      buffers.clear();
    }
  }
}  // namespace squark
