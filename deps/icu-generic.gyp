# Copyright (c) IBM Corporation and Others. All Rights Reserved.
# very loosely based on icu.gyp from Chromium:
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


{
  'variables': {
    'icu_src_derb': [ 'icu/source/tools/genrb/derb.c' ],
  },
  'targets': [
    {
      # a target to hold uconfig defines.
      # for now these are hard coded, but could be defined.
      'target_name': 'icu_uconfig',
      'type': 'none',
      'toolsets': [ 'host','target' ],
      'direct_dependent_settings': {
        'defines': [
          'UCONFIG_NO_LEGACY_CONVERSION=1',
          'UCONFIG_NO_IDNA=1',
          'UCONFIG_NO_TRANSLITERATION=1',
          'UCONFIG_NO_SERVICE=1',
          'UCONFIG_NO_REGULAR_EXPRESSIONS=1',
          'U_ENABLE_DYLOAD=0',
          'U_STATIC_IMPLEMENTATION=1',
          # TODO(srl295): reenable following pending
          # https://code.google.com/p/v8/issues/detail?id=3345
          # (saves some space)
          'UCONFIG_NO_BREAK_ITERATION=0',
        ],
      }
    },
    {
      # a target to hold common settings.
      # make any target that is ICU implementation depend on this.
      'target_name': 'icu_implementation',
      'toolsets': [ 'host','target' ],
      'type': 'none',
      'direct_dependent_settings': {
        'conditions': [
          [ 'os_posix == 1 and OS != "mac" and OS != "ios"', {
            'cflags': [ '-Wno-deprecated-declarations' ],
            'cflags_cc': [ '-frtti' ],
          }],
          ['OS == "mac" or OS == "ios"', {
            'xcode_settings': {'GCC_ENABLE_CPP_RTTI': 'YES' },
          }],
          ['OS == "win"', {
            'msvs_settings': {
              'VCCLCompilerTool': {'RuntimeTypeInfo': 'true'},
            }
          }],
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeTypeInfo': 'true',
            'ExceptionHandling': '1',
          },
        },
        'configurations': {
          # TODO: why does this need to be redefined for Release and Debug?
          # Maybe this should be pushed into common.gypi with an "if v8 i18n"?
          'Release': {
            'msvs_settings': {
              'VCCLCompilerTool': {
                'RuntimeTypeInfo': 'true',
                'ExceptionHandling': '1',
              },
            },
          },
          'Debug': {
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
      'toolsets': [ 'host','target' ],
      'sources': [
        '<@(icu_src_i18n)'
      ],
      'include_dirs': [
        'icu/source/i18n',
      ],
      'defines': [
        'U_I18N_IMPLEMENTATION=1',
      ],
      'dependencies': ['icuucx', 'icu_implementation', 'icu_uconfig'],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/i18n',
        ],
      },
      'export_dependent_settings': ['icuucx'],
    },
    # this is only built for derb..
    {
      'target_name': 'icuio',
      'type': '<(library)',
      'toolsets': [ 'host','target' ],
      'sources': [
        '<@(icu_src_io)'
      ],
      'include_dirs': [
        'icu/source/io',
      ],
      'defines': [
        'U_IO_IMPLEMENTATION=1',
      ],
      'dependencies': ['icuucx','icui18n','icu_implementation','icu_uconfig'],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/io',
        ],
      },
      'export_dependent_settings': ['icuucx','icui18n'],
    },
    # This exports actual ICU data
    {
      'target_name': 'icudata',
      'type': '<(library)',
      'toolsets': [ 'host','target' ],
      'conditions': [
        [ 'OS == "win"', {
          'conditions': [
            [ 'icu_full == "true"', { # and OS=win
              # full data - just build the full data file, then we are done.
              'sources': [ '<(SHARED_INTERMEDIATE_DIR)/icudt<(icu_ver_major)<(icu_endianness)_dat.obj' ],
              'dependencies': ['genccode'],
              'actions': [
                {
                  'action_name': 'icudata',
                  'inputs': [ '<(icu_data_in)' ],
                  'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/icudt<(icu_ver_major)<(icu_endianness)_dat.obj' ],
                  'action': [ '<(PRODUCT_DIR)/genccode',
                              '-o',
                              '-d','<(SHARED_INTERMEDIATE_DIR)',
                              '-n','icudata',
                              '-e','icudt<(icu_ver_major)',
                              '<@(_inputs)' ],
                },
              ],
            }, { # icu_full == FALSE and OS == win
              # link against stub data primarily
              # then, use icupkg and genccode to rebuild data
              'dependencies': ['icustubdata', 'genccode','icupkg','genrb','iculslocs'],
              'export_dependent_settings': ['icustubdata'],
              'actions': [
                {
                  # trim down ICU
                  'action_name': 'icutrim',
                  'inputs': [ '<(icu_data_in)', 'icu-trim/icu_small.json' ],
                  'outputs': [ '../out/icutmp/icudt<(icu_ver_major)<(icu_endianness).dat' ],
                  'action': [ 'icu-trim/icutrim.py',
                              '-P', '../<(CONFIGURATION_NAME)',
                              '-D', '<(icu_data_in)',
                              '--delete-tmp',
                              '-T', '../out/icutmp',
                              '-F', 'icu-trim/icu_small.json',
                              '-O', 'icudt<(icu_ver_major)<(icu_endianness).dat',
                              '-v' ],
                },
                {
                  # build final .dat -> .obj
                  'action_name': 'genccode',
                  'inputs': [ '../out/icutmp/icudt<(icu_ver_major)<(icu_endianness).dat' ],
                  'outputs': [ '../out/icudt<(icu_ver_major)<(icu_endianness)_dat.obj' ],
                  'action': [ '../<(CONFIGURATION_NAME)/genccode',
                              '-o',
                              '-d', '../out/',
                              '-n','icudata',
                              '-e','icusmdt<(icu_ver_major)',
                              '<@(_inputs)' ],
                },
              ],
              # This file actually contains the small ICU data - go figure.
              'sources': [ '../out/icudt<(icu_ver_major)<(icu_endianness)_dat.obj' ],
            } ] ], #end of OS==win and icu_full == false
        }, { # OS != win
          'conditions': [
            [ 'icu_full == "true"', {
              # full data - just build the full data file, then we are done.
              'sources': [ '<(SHARED_INTERMEDIATE_DIR)/icudt<(icu_ver_major)_dat.c' ],
              'dependencies': ['genccode', 'icu_implementation', 'icu_uconfig'],
              'include_dirs': [
                'icu/source/common',
              ],
              'actions': [
                # HUGE TODO - use .s or .o not .c ..

                # TODO: needed for endianness swap
                {
                   'action_name': 'icupkg',
                   'inputs': [ '<(icu_data_in)' ],
                   'outputs':[ '<(SHARED_INTERMEDIATE_DIR)/icudt<(icu_ver_major).dat' ],
                   'action': [ 'cp', #'<(PRODUCT_DIR)/icupkg',
                               #'-t<(icu_endianness)',
                               '<@(_inputs)',
                               '<@(_outputs)',
                             ], #TODO: actually swap.
                },
                {
                  'action_name': 'icudata',
                  'inputs': [ '<(SHARED_INTERMEDIATE_DIR)/icudt<(icu_ver_major).dat' ],
                  'outputs':[ '<(SHARED_INTERMEDIATE_DIR)/icudt<(icu_ver_major)_dat.c' ],
                  'action': [ '<(PRODUCT_DIR)/genccode',
                              '-e','icudt<(icu_ver_major)',
                              '-d','<(SHARED_INTERMEDIATE_DIR)',
                              '-f','icudt<(icu_ver_major)_dat',
                              '<@(_inputs)' ],
                },
              ], # end actions
            }, { # icu_full == false ( and OS != win )
              # link against stub data (as primary data)
              # then, use icupkg and genccode to rebuild small data
              'dependencies': ['icustubdata', 'genccode','icupkg','genrb','iculslocs'],
              'export_dependent_settings': ['icustubdata'],
              'actions': [
                {
                  # trim down ICU
                  'action_name': 'icutrim',
                  'inputs': [ '<(icu_data_in)', 'icu-trim/icu_small.json' ],
                  'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/icutmp/icudt<(icu_ver_major)<(icu_endianness).dat' ],
                  'action': [ 'icu-trim/icutrim.py',
                              '-P', '<(PRODUCT_DIR)',
                              '-D', '<(icu_data_in)',
                              '--delete-tmp',
                              '-T', '<(SHARED_INTERMEDIATE_DIR)/icutmp',
                              '-F', 'icu-trim/icu_small.json',
                              '-O', 'icudt<(icu_ver_major)<(icu_endianness).dat',
                              '-v' ],
                }, {
                  # build final .dat -> .obj
                  'action_name': 'genccode',
                  'inputs': [ '<(SHARED_INTERMEDIATE_DIR)/icutmp/icudt<(icu_ver_major)<(icu_endianness).dat' ],
                  'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/icusmdt<(icu_ver_major)_dat.c' ],
                  'action': [ '<(PRODUCT_DIR)/genccode',
                              '-d', '<(SHARED_INTERMEDIATE_DIR)',
                              '-n','icusmdt<(icu_ver_major)',
                              '-e','icusmdt<(icu_ver_major)',
                              '-f','icusmdt<(icu_ver_major)_dat',
                              '<@(_inputs)' ],
                },
              ],
              # This file contains the small ICU data
              'sources': [ '<(SHARED_INTERMEDIATE_DIR)/icusmdt<(icu_ver_major)_dat.c' ],
            }]], # end icu_full
        }]], # end OS != win
    }, # end icudata

    # icustubdata:
    # this means "no data". It's a tiny (~1k) symbol with no ICU data in it.
    # tools must link against it as they are generating the full data.
    {
      'target_name': 'icustubdata',
      'type': '<(library)',
      'toolsets': [ 'host','target' ],
      'dependencies': ['icu_implementation'],
      'sources': [
        '<@(icu_src_stubdata)'
      ],
      'include_dirs': [
        'icu/source/common',
      ],
    },
    # this target is for v8 consumption.
    # it is icuuc + stubdata
    {
      'target_name': 'icuuc',
      'type': 'none',
      'toolsets': [ 'host','target' ],
      'dependencies': ['icuucx', 'icudata'],
      'export_dependent_settings': ['icuucx', 'icudata'],
    },
    # This is the 'real' icuuc.
    # tools can depend on 'icuuc + stubdata'
    {
      'target_name': 'icuucx',
      'type': '<(library)',
      'dependencies': ['icu_implementation','icu_uconfig'],
      'toolsets': [ 'host','target' ],
      'sources': [
        '<@(icu_src_common)'
      ],
      'include_dirs': [
        'icu/source/common',
      ],
      'defines': [
        'U_COMMON_IMPLEMENTATION=1',
      ],
      'export_dependent_settings': ['icu_uconfig'],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/common',
        ],
        'conditions': [
          [ 'OS=="win"', {
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
      'toolsets': [ 'host','target' ],
      'dependencies': ['icuucx','icui18n','icustubdata'],
      'sources': [
        '<@(icu_src_tools)'
      ],
      'include_dirs': [
        'icu/source/tools/toolutil',
      ],
      'defines': [
        'U_TOOLUTIL_IMPLEMENTATION=1',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'icu/source/tools/toolutil',
        ],
      },
      'export_dependent_settings': ['icuucx','icui18n','icustubdata'],
    },
    # This is needed to rebuild .res files from .txt,
    # or to build index (res_index.txt) files for small-icu
    {
      'target_name': 'genrb',
      'type': 'executable',
      'toolsets': [ 'host','target' ],
      'dependencies': ['icutools','icuucx','icui18n'],
      'sources': [
        '<@(icu_src_genrb)'
      ],
      # derb is a separate executable
      'sources!': [
        '<@(icu_src_derb)',
      ],
    },
    # {
    #   'target_name': 'derb',
    #   'type': 'executable',
    #   'dependencies': ['icutools','icuucx','icui18n','icuio'],
    #   'sources': [
    #     '<@(icu_src_derb)',
    #   ],
    # },
    # experimental.
    {
      'target_name': 'iculslocs',
      'toolsets': [ 'host','target' ],
      'type': 'executable',
      'dependencies': ['icutools','icuucx','icui18n','icuio'],
      'sources': [
        'icu-trim/iculslocs.cpp',
      ],
    },
    # This is used to package, unpackage, repackage .dat files
    # and convert endianesses
    {
      'target_name': 'icupkg',
      'toolsets': [ 'host','target' ],
      'type': 'executable',
      'dependencies': ['icutools','icuucx','icui18n'],
      'sources': [
        '<@(icu_src_icupkg)'
      ],
    },
    # this is used to convert .dat directly into .obj
    {
      'target_name': 'genccode',
      'toolsets': [ 'host','target' ],
      'type': 'executable',
      'dependencies': ['icutools','icuucx','icui18n'],
      'sources': [
        '<@(icu_src_genccode)',
        'icu-trim/no-op.cc',
      ],
    },
  ],
}
