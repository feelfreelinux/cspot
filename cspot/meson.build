project('cspot', ['cpp', 'c'])
ssldep = dependency('libssl')
cryptodep = dependency('libcrypto')

incdir = include_directories('include')
sources = ['src/main.cpp', 'src/hmac.c', 'src/base64.cpp', 'src/mongoose.c', 'src/sha1.c']

deps = [ssldep, cryptodep]

executable('cspot', sources, dependencies: deps, include_directories : incdir)
