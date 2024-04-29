set(MODULES
  aac
  account
  alsa
  amr
  aptx
  aubridge
  auconv
  audiounit
  aufile
  auresamp
  ausine
  av1
  avcapture
  avcodec
  avfilter
  avformat
  codec2
  cons
  contact
  coreaudio
  ctrl_dbus
  ctrl_tcp
  debug_cmd
  directfb
  dshow
  dtls_srtp
  ebuacip
  echo
  evdev
  fakevideo
  g711
  g722
  g7221
  g726
  gRPC
  gst
  gtk
  gzrtp
  httpd
  httpreq
  ice
  jack
  l16
  menu
  mixausrc
  mixminus
  mpa
  mqtt
  multicast
  mwi
  natpmp
  netroam
  opensles
  opus
  opus_multistream
  pcp
  pipewire
  plc
  portaudio
  presence
  pulse
  rtcpsummary
  sdl
  selfview
  serreg
  snapshot
  sndfile
  sndio
  srtp
  stdio
  stun
  swscale
  syslog
  turn
  uuid
  v4l2
  vidbridge
  vidinfo
  vp8
  vp9
  vumeter
  webrtc_aec
  webrtc_aecm
  wincons
  winwave
  x11

  CACHE STRING "List of modules like 'turn;pipewire;alsa'"
)


if(DEFINED EXTRA_MODULES)
  list(APPEND MODULES ${EXTRA_MODULES})
endif()
