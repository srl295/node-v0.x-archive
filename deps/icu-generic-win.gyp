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
          'target_name': 'icui18n',
          'type': '<(library)',
          'sources': [
            '<@(icu_src_i18n)'
          ],
          'defines': [
            'U_I18N_IMPLEMENTATION=1',
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeTypeInfo': 'true',
              'ExceptionHandling': '1',
            },
          },
          'dependencies': ['icuuc'],
          'direct_dependent_settings': {
            'include_dirs': [
              'icu/source/i18n',
              'deps/icu/i18n',
            ],
          },
          'export_dependent_settings': ['icuuc'],
        },
        {
            'target_name': 'icudata',
            'type': 'none',
	    'dependencies': ['icustubdata'],
        },
        {
            'target_name': 'icustubdata',
            'type': '<(library)',
	    'sources': [
              '<@(icu_src_stubdata)'
	    ],
            'include_dirs': [
              'icu/source/common',
            ],
        },
        {
          'target_name': 'icuuc',
          'type': '<(library)',
          'dependencies': ['icudata'],
          'sources': [
            '<@(icu_src_common)'
          ],
          'defines': [
            'U_COMMON_IMPLEMENTATION=1',
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'RuntimeTypeInfo': 'true',
              'ExceptionHandling': '1',
            },
          },
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
            },
            'export_dependent_settings': ['icudata'],
        },
    ],
}
