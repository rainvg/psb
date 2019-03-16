load('//:buckaroo_macros.bzl', 'buckaroo_deps')

# Configuration

mode = read_config('build', 'mode', 'release')

if mode not in ['release', 'debug']:
    print "Build mode not recognized, using the default release mode."
    mode = 'release'

# PSB sources & headers

psb = {
    'sources': glob([
      'psb/src/**/*.cpp',
    ]),
    'headers': subdir_glob([
      ('psb/src', '**/*.h'),
      ('psb/src', '**/*.hpp'),
    ]),
    'flags': {
        'common': {},
        'debug': {},
        'release': {}
    }
}

# PSB flags

psb['flags']['common'] = {
    'compiler': ['-std=c++2a', '-stdlib=libc++', '-fcoroutines-ts'],
    'linker': ['-stdlib=libc++', '-lc++abi', '-pthread']
}

psb['flags']['debug'] = {
    'compiler': ['-O0', '-g'],
    'linker': []
}

psb['flags']['release'] = {
    'compiler': ['-O3'],
    'linker': []
}

# Test sources & headers

test = {
    'sources': glob([
      'psb/test/**/*.cpp',
    ]),
    'headers': subdir_glob([
      ('psb/test', '**/*.h'),
      ('psb/test', '**/*.hpp'),
    ])
}

# Build targets

cxx_library(
  name = 'psb',
  header_namespace = 'psb',
  srcs = psb['sources'],
  exported_headers = psb['headers'],
  compiler_flags = psb['flags']['common']['compiler'] + psb['flags'][mode]['compiler'],
  linker_flags = psb['flags']['common']['linker'] + psb['flags'][mode]['linker'],
  deps = buckaroo_deps(),
  visibility = ['PUBLIC']
)

cxx_binary(
  name = 'test',
  srcs = test['sources'],
  headers =  test['headers'],
  compiler_flags = psb['flags']['common']['compiler'] + psb['flags'][mode]['compiler'],
  linker_flags = psb['flags']['common']['linker'] + psb['flags'][mode]['linker'],
  deps = [":psb"],
  visibility = []
)
