## libyuri

Libyuri is a framework for node-based video/audio processing.
It was originally developed for real-time video processing, but it can be used for batch video processing. Audio suppoort is sparse, but sufficient for audio playback. The framework also supports simple scripting and user inputs, allowing the processing pipeline to be controled during execution.



The framework consists of several parts:
 - library libyuri2.8_core
   - The main library providing the infrastructure for libyuri applications
 - modules
   - Dynamically loaded modules provide interface to input/output devices and all the processing functionality.
 - executables
   - The framework has several standalone executables that allow usage of the functionality without the need to write a custom application.
 - helper libraries
   - Helper libraries contain functions to ease developement of new modules.
 
 
### Quick examples
 The bundled application **yuri_simple** can be used to quickly create simple processing graphs.

#### Show video from a webcam in a GLX window
 ```bash
 yuri_simple v4l2source glx_window
 ```

Quite simple, huh?

Ok, let's say, we want different webcam (*/dev/video1*), using MJPEG (because uncompressed video has low framerate), with resolution 1280x720 at 30fps. And then flip it around X axis so it works like a mirror!

```bash
 yuri_simple v4l2source[path=/dev/video1,format=MJPEG,resolution=1280x720,fps=30] glx_window[flip_x=TRUE]
 ```

##### More nodes in graph
Yup, it can get longer.....
```bash
yuri_simple v4l2source[resolution=320x240] render_text[text="Frame %i",position=0x100,color="#006677"]  pad[resolution=400x300] rotate[angle=180] glx_window
```

#### Screen capture
Another example, capturing screen content and displaying it in glx window. The **frame_info** node in between simply outputs video format to the console.

```bash
yuri_simple screen frame_info glx_window
```



### XML syntax

