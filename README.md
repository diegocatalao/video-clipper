# Squark

This is a very small project to transform a video stream into a small
thumbnails. It`s can be useful if you need something like a YouTube thumbnail 
preview.

Must be improved with a MSE calculation of frames diff.

On smart TV's you can use this to handle backpressure on your CDNs / servers so that it doesn't require as many thumbnails. You can this images **inside** of your tv. That`s great, right?!

This project is totally based on Gstreamer, its a plugin and you will be 
set it on your pipeline.

I just wrote this with C++ to use the OpenCV. This is good enough.

## Dependencies
This project depends on `GStreamer`, `OpenCV4` and good eyes.

On MacOS:
```sh
$ brew install gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad 
$ brew install opencv
```

## Build

```bash
$ mkdir build
$ cd build
$ cmake .. && make
```

## Usage

On the root of this projects, do:

```bash
$ export GST_PLUGIN_PATH=${pwd}/build 
$ GST_DEBUG=squark:7 gst-launch-1.0 videotestsrc ! videoconvert ! squark slots=4 ! autovideosink
```

Its will create some images from a videofake source.

## Contributing

You cannot contribute with this because I'm not do maintain this project. Right now its just a nice and funny solution to 💛 PlutoTV 💛

## License

[MIT](https://choosealicense.com/licenses/mit/)
