_sources = [
  'gstplugin.c',
  'gstpowereventemitter.c'
]

gsteventemitters = library('gsteventemitters',
  _sources,
  c_args : gst_plugins_bad_args,
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstpbutils_dep, gstaudio_dep, gstvideo_dep, gstfft_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gsteventemitters, install_dir : plugins_pkgconfig_install_dir)
