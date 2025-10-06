#pragma once

#include <gst/gst.h>

#include <mutex>
#include <queue>

namespace squark {
  class Bucket {
   private:
    std::queue<GstBuffer*> queue;  /**< a simple queue for this ingetion and digestion  */
    mutable std::mutex     mutex;  /**< a mutex to block queue when is not possible to operate */
    int                    size;   /**< the maximum size of the queue  */
    std::string            prefix; /**< the the prefix name of the output image file */
    int                    slots;  /**< the size of slots in a single image */
    int                    width;  /**< the video width from info caps */
    int                    height; /**< the video height from info caps */
    bool                   keep_alive{false}; /**< keep the digestion thread alive */
    double                 mse{0.0};
    double                 last_pts{0.0};
    uint8_t*               preview{nullptr};

    /**
     *
     */
    void calculate_mse_and_push(GstBuffer* buffer);

    /**
     * @brief blocking dequeue intended for the dedicated consumer thread
     */
    void digest();

   public:
    /**
     * @brief construct a Bucket with some primitive params
     *
     * @param size maximum number of buffers allowed in the queue
     * @param prefix the prefix name of the output files
     * @param retention the time of an image can be exists on disc
     * @param slots the size of slots of a single image
     */
    Bucket(int size, std::string prefix, int slots, int w, int h);

    /**
     * @brief construct a Bucket with some gstreamer params
     *
     * @param size maximum number of buffers allowed in the queue
     * @param prefix the prefix name of the output files
     * @param slots the size of slots of a single image
     */
    Bucket(int size, gchar* prefix, guint slots, gint w, gint h);

    /**
     * @brief pushes a new buffer into the bucket
     *
     * @details push function is thread safe but can ignore some frames when is full
     * @param buffer GstBuffer* to push into the queue
     */
    void ingest(GstBuffer* buffer);

    /**
     * @brief start a single thread to consume all frames
     * @details can be called more times but the effect is ignored
     */
    void start_digest_thread();

    /**
     * @brief shutdown the thread and wait for all operations stop
     */
    void shutdown();
  };
}  // namespace squark
