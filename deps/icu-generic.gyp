# Copyright (c) IBM Corporation and Others. All Rights Reserved.
# based on icu.gyp from Chromium
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# ICU linkage is from v8/src/d8.gyp which includes  icui18n, icuuc, and icudata respectively.


{
  'includes': [
# DIDNTWORK
#   '<(PRODUCT_DIR)/deps/icu/icu-config.gypi',
    '../out/Release/deps/icu/icu-config.gypi',
  ],
  'targets': [
    {
      'toolsets': [ 'host','target' ],
      'target_name': 'icui18n',
      'type': 'none',
      'dependencies': ['icuuc'],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/i18n',
          '<(PRODUCT_DIR)/deps/icu/i18n',
        ],
        'link_settings': {
          'library_dirs': [
            '<(PRODUCT_DIR)/deps/icu/lib',
          ],
          'libraries': [
            '-licui18n',
          ],
        },
      },
      'export_dependent_settings': ['icuuc'],
    },
    {
      'target_name': 'icudata',
      'toolsets': [ 'host','target' ],
      'type': 'none',
      'direct_dependent_settings': {
        'link_settings': {
          'library_dirs': [
            '<(PRODUCT_DIR)/deps/icu/lib',
          ],
          'libraries': [
            '-licudata',
          ],
        },
        'conditions': [
          [ 'icu_full=="false"', {
            'link_settings': {
              'libraries': [
                '-licusmdata',
              ],
              'library_dirs': [
                '<(PRODUCT_DIR)/deps/icu/stubdata',
              ],
            }
          }, ]
        ],
      },
    },
    {
      'target_name': 'icuuc',
      'toolsets': [ 'host','target' ],
      'type': 'none',
      'dependencies': ['icudata'],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/common',
          '<(PRODUCT_DIR)/deps/icu/common',
        ],
        'defines': [
          'UCONFIG_NO_LEGACY_CONVERSION=<(icu_UCONFIG_NO_LEGACY_CONVERSION)',
          'UCONFIG_NO_IDNA=<(icu_UCONFIG_NO_IDNA)',
          'UCONFIG_NO_BREAK_ITERATION=<(icu_UCONFIG_NO_BREAK_ITERATION)',
          'UCONFIG_NO_TRANSLITERATION=<(icu_UCONFIG_NO_TRANSLITERATION)',
          'UCONFIG_NO_REGULAR_EXPRESSIONS=<(icu_UCONFIG_NO_REGULAR_EXPRESSIONS)',
          'UCONFIG_SRL_NO_TEST_API=<(icu_UCONFIG_SRL_NO_TEST_API)',
        ],
        'link_settings': {
          'library_dirs': [
            '<(PRODUCT_DIR)/deps/icu/lib',
          ],
          'libraries': [
            '-licuuc',
          ],
        },
      },
      'export_dependent_settings': ['icudata'],
    },
  ],
}
