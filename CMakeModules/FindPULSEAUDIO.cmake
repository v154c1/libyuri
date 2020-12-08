find_path(PULSEAUDIO_INCLUDE_DIR pulse/pulseaudio.h PATHS "/usr/include")
find_library(PULSEAUDIO_LIBRARY NAMES pulse libpulse PATHS "/usr/lib/x86_64-linux-gnu" "/usr/lib/arm-linux-gnueabihf")
find_library(PULSEAUDIO_SIMPLE_LIBRARY NAMES pulse-simple PATHS "/usr/lib/x86_64-linux-gnu" "/usr/lib/arm-linux-gnueabihf")
find_library(PULSEAUDIO_MAINLOOP_LIBRARY NAMES pulse-mainloop-glib libpulse-mainloop-glib PATHS "/usr/lib/x86_64-linux-gnu" "/usr/lib/arm-linux-gnueabihf")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PULSEAUDIO
                                  REQUIRED_VARS PULSEAUDIO_INCLUDE_DIR PULSEAUDIO_LIBRARY PULSEAUDIO_SIMPLE_LIBRARY PULSEAUDIO_MAINLOOP_LIBRARY)
