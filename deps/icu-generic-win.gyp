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
      'dependencies': ['icuuc', 'icu_implementation'],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/i18n',
          'deps/icu/i18n',
        ],
      },
      'export_dependent_settings': ['icuuc'],
    },
    # {
    #   'target_name': 'icudata',
    #   'type': 'none',
    #   'dependencies': ['icustubdata'],
    #   'export_dependent_settings': ['icustubdata'],
    # },
    {
      'target_name': 'icudata',
      'type': '<(library)',
      'dependencies': ['icu_implementation'],
      'sources': [
        '<@(icu_src_stubdata)'
      ],
      'include_dirs': [
        'icu/source/common',
      ],
      # 'direct_dependent_settings': {
      #   'libraries': [ '-licudata' ],
      # },
    },
    {
      'target_name': 'icuuc',
      'type': '<(library)',
      'dependencies': ['icudata','icu_implementation'],
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
      'export_dependent_settings': ['icudata'],
    },
  ],
}
