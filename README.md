# pixelstick
- my pixelstick using arduino and library NeoPixel

# Arduino Interface
- PixelStick
- Version: 1.0.0  https://github.com/hankso/MyPixelStick
- Usage: [options ...]
- Options:
    - run:    run displaying photos
    - stop:   stop displaying and keep main-thread rest
    - push:   indicator for transmitting data via blueteeth
    - time+n: timeInterval plus num
    - time-n: timeInterval minus num
    - ls:     print a tree structure of files in SD card
    - next:   display next photo in SD card
    - prev:   display previous photo
    - help:   show this message
    - clear:  set all pixels off

![arduino interface](https://github.com/hankso/MyPixelStick/blob/master/Screenshot%20from%202017-11-24%2001.57.24.jpeg?raw=true)


# Data files
txt file in SD card and data sent through bluetooth must have a header and an end indicator, which looks like this:
>#filename.txt#
>i1 r g b
>i2 r g b
>......
#255#255#
