ffmpegSw is wrapper for ffmpeg library from version 4.1.

Version 1.0.0 has support playing video and audio. It supports changing the parameters of video and audio conversion during work.
ffmpegSw is thread safe and supports two operation's modes: playing with reconnect will be try to reconnect to stream until you close file, normal mode will stop reading and send end of file with any connection error.
You can use AVffmpegWrapper instance how single gate for AV streams or you can use AVFileContext for each stream directly.

Don't forget to put shared lybreryes into your target directory (for windows)!
