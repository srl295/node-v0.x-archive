# Copyright (c) IBM Corporation and Others. All Rights Reserved.
# based on icu.gyp from Chromium
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


{
  # hard-coded for now. move to config.gypi?
  'variables': { 'icu_UCONFIG_NO_LEGACY_CONVERSION' : 1,
                 'icu_UCONFIG_NO_IDNA': 1,
                 'icu_UCONFIG_NO_BREAK_ITERATION' : 0,
                 'icu_UCONFIG_NO_TRANSLITERATION' : 1,
                 'icu_UCONFIG_NO_REGULAR_EXPRESSIONS' : 1,
                 'icu_UCONFIG_SRL_NO_TEST_API' : 1 
               },
  'targets': [
    {
      # a target to hold common settings
      'target_name': 'icu_implementation',
      'type': 'none',
      'direct_dependent_settings': {
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeTypeInfo': 'true',
            'ExceptionHandling': '1',
          },
        },
        'configurations': {
          # TODO: why does this need to be redefined for Release?
          # Maybe this should be pushed into common.gypi with an "if v8 i18n"?
          'Release': {
            'msvs_settings': {
              'VCCLCompilerTool': {
                'RuntimeTypeInfo': 'true',
                'ExceptionHandling': '1',
              },
            },
          },
        },
        'defines': [
          'U_ATTRIBUTE_DEPRECATED=',
          '_CRT_SECURE_NO_DEPRECATE=',
	  'U_STATIC_IMPLEMENTATION=1',
        ],
      },
    },
    {
      'target_name': 'icui18n',
      'type': '<(library)',
      'sources': [
        '<@(icu_src_i18n)'
      ],
      'defines': [
        'U_I18N_IMPLEMENTATION=1',
      ],
      'dependencies': ['icuucx', 'icu_implementation'],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/i18n',
          'deps/icu/i18n',
        ],
      },
      'export_dependent_settings': ['icuucx'],
    },
    # TODO(srl295): for 'stub ICU' don't depend on genccode but depend on stubdata.
    # TODO(srl295): for 'full ICU' depend on genccode AND full data
    # TODO(srl295): for 'small ICU' depend on genccode, icupkg, and a cast of 1000s
    {
      'target_name': 'icudata',
      'type': '<(library)',
      'dependencies': ['genccode'],
      'actions': [
        {
          'action_name': 'icudata',
          'inputs': [ 'icu/source/data/in/icudt53l.dat' ], # TODO: make a param obviously.
          'outputs': [ '../out/icudt53l_dat.obj' ], ## TODO fix
          'action': [ '../Release/genccode -o -d ../out/ -n icudata -e icudt53 <@(_inputs)' ],
        },
      ],
      'sources': [ '../out/icudt53l_dat.obj' ],
    },
    # this means "no data". It's a tiny (~1k) symbol with no ICU data in it.
    # tools must link against it as they are generating the full data.
    {
      'target_name': 'icustubdata',
      'type': '<(library)',
      'dependencies': ['icu_implementation'],
      'sources': [
        '<@(icu_src_stubdata)'
      ],
      'include_dirs': [
        'icu/source/common',
      ],
    },
    # this target is for d8 consumption.
    # it is icuuc + stubdata
    {
      'target_name': 'icuuc',
      'type': 'none',
      'dependencies': ['icuucx', 'icudata' ],
      'export_dependent_settings': ['icuucx', 'icudata' ],
    },
    # This is the 'real' icuuc.
    # tools can depend on 'icuuc + stubdata'
    {
      'target_name': 'icuucx',
      'type': '<(library)',
      'dependencies': ['icu_implementation'],
      'sources': [
        '<@(icu_src_common)'
      ],
      'defines': [
        'U_COMMON_IMPLEMENTATION=1',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/common',
          'deps/icu/common',
        ],
        'defines': [
          'UCONFIG_NO_LEGACY_CONVERSION=<(icu_UCONFIG_NO_LEGACY_CONVERSION)',
          'UCONFIG_NO_IDNA=<(icu_UCONFIG_NO_IDNA)',
          'UCONFIG_NO_BREAK_ITERATION=<(icu_UCONFIG_NO_BREAK_ITERATION)',
          'UCONFIG_NO_TRANSLITERATION=<(icu_UCONFIG_NO_TRANSLITERATION)',
          'UCONFIG_NO_REGULAR_EXPRESSIONS=<(icu_UCONFIG_NO_REGULAR_EXPRESSIONS)',
          'UCONFIG_SRL_NO_TEST_API=<(icu_UCONFIG_SRL_NO_TEST_API)',
          'U_STATIC_IMPLEMENTATION=1',
        ],
        'conditions': [
          [ 'host_arch=="x64"', {
            'link_settings': {
              'libraries': [ '-lAdvAPI32.Lib','-lUser32.lib' ], # should be 64?
            },
          }],
          [ 'host_arch=="ia32"', {
            'link_settings': {
              'libraries': [ '-lAdvAPI32.Lib','-lUser32.lib' ],
            },
          }],
        ],
      },
    },
    {
      'target_name': 'icutools',
      'type': '<(library)',
      'dependencies': ['icuucx','icui18n','icustubdata'],
      'sources': [
        '<@(icu_src_tools)'
      ],
      'defines': [
        'U_TOOLUTIL_IMPLEMENTATION=1',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/tools/toolutil',
          'deps/icu/toolutil',
        ],
      },
      'export_dependent_settings': ['icuucx','icui18n','icustubdata'],
    },
    # TODO: may not need this, at least for full-icu.
    # It is needed to rebuild .res files from .txt,
    # or to build index (res_index.txt) files, though.
    {
      'target_name': 'genrb',
      'type': 'executable',
      'dependencies': ['icutools','icuucx','icui18n'],
      'sources': [
        '<@(icu_src_genrb)'
      ],
    },
    # This is used to package, unpackage, repackage .dat files
    # and convert endianesses
    {
      'target_name': 'icupkg',
      'type': 'executable',
      'dependencies': ['icutools','icuucx','icui18n'],
      'sources': [
        '<@(icu_src_icupkg)'
      ],
    },
    # this is used to convert .dat directly into .obj. Do not pass go, do not collect US$200.
    {
      'target_name': 'genccode',
      'type': 'executable',
      'dependencies': ['icutools','icuucx','icui18n'],
      'sources': [
        '<@(icu_src_genccode)'
      ],
    },
  ],
}
