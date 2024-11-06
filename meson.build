project('test_project', 'cpp', 
  default_options : ['cpp_std=c++23', 'buildtype=debug'])

# this is to deal with it complaining about my union initialization.
add_project_arguments('-Wno-missing-braces', language: 'cpp')

# Set up include directories
inc_dir = include_directories('inc')

# External SDL3 static library
sdl3_lib = declare_dependency(
  include_directories : 'inc',
  link_args : ['../external/static_libraries/SDL3.lib', '-lgdi32', '-lopengl32']
)

# Add SDL3 as a dependency (optional step in case you want to fetch SDL3 with subprojects)
# SDL3 dependency should normally be handled by a pkg-config file if dynamic linking is desired
# sdl3_dep = dependency('sdl3', required : true)

# Build test_client_server
executable('test_client_server', 
  'tst/client_server.cc',
  install : false
)

# Build AABB_test
executable('AABB_test',
  'tst/AABB_test.cc',
  include_directories : inc_dir,
  install : false
)

# Build baseline_sdl_test
executable('baseline_sdl_test',
  'tst/SDL_baseline_test.cc',
  include_directories : inc_dir,
  dependencies : [sdl3_lib],
  link_args : ['-Wl,/SUBSYSTEM:WINDOWS'],
  install : false
)

# Build sdl_test
executable('sdl_test',
  ['tst/SDL_test.cc', 'src/glad.cpp'],
  include_directories : inc_dir,
  dependencies : [sdl3_lib],
  link_args : ['-Wl,/SUBSYSTEM:CONSOLE', '-Wno-missing-braces'],
  install : false
)

# Build server_test
executable('server_test',
  'tst/server_test.cc',
  include_directories : inc_dir,
  install : false
)

# Build server_test
executable('font_test',
  'tst/font_test.cc',
  include_directories : inc_dir,
  install : false
)