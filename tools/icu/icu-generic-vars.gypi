{
  'variables': {
    'icu_src_derb': [ '../../deps/icu/source/tools/genrb/derb.c' ],

    'icu_src_i18n_skip_54': [
      ## Strip out the following for ICU 54 only.
      ## add more conditions in the future?
      ## if your compiler can dead-strip, this will
      ## make ZERO difference to binary size.
      ## Made ICU-specific for future-proofing.
      
      # alphabetic index
      '../../deps/icu/source/i18n/alphaindex.cpp',
      # BOCSU
      # misc
      '../../deps/icu/source/i18n/dtitvfmt.cpp',
      '../../deps/icu/source/i18n/dtitvinf.cpp',
      '../../deps/icu/source/i18n/dtitv_impl.h',
      '../../deps/icu/source/i18n/quantityformatter.cpp',
      '../../deps/icu/source/i18n/quantityformatter.h',
      '../../deps/icu/source/i18n/regexcmp.cpp',
      '../../deps/icu/source/i18n/regexcmp.h',
      '../../deps/icu/source/i18n/regexcst.h',
      '../../deps/icu/source/i18n/regeximp.cpp',
      '../../deps/icu/source/i18n/regeximp.h',
      '../../deps/icu/source/i18n/regexst.cpp',
      '../../deps/icu/source/i18n/regexst.h',
      '../../deps/icu/source/i18n/regextxt.cpp',
      '../../deps/icu/source/i18n/regextxt.h',
      '../../deps/icu/source/i18n/region.cpp',
      '../../deps/icu/source/i18n/region_impl.h',
      '../../deps/icu/source/i18n/reldatefmt.cpp',
      '../../deps/icu/source/i18n/reldatefmt.h',
      '../../deps/icu/source/i18n/measfmt.h',
      '../../deps/icu/source/i18n/measfmt.cpp',
      '../../deps/icu/source/i18n/scientificformathelper.cpp',
      '../../deps/icu/source/i18n/tmunit.cpp',
      '../../deps/icu/source/i18n/tmutamt.cpp',
      '../../deps/icu/source/i18n/tmutfmt.cpp',
      '../../deps/icu/source/i18n/uregex.cpp',
      '../../deps/icu/source/i18n/uregexc.cpp',
      '../../deps/icu/source/i18n/uregion.cpp',
      '../../deps/icu/source/i18n/uspoof.cpp',
      '../../deps/icu/source/i18n/uspoof_build.cpp',
      '../../deps/icu/source/i18n/uspoof_conf.cpp',
      '../../deps/icu/source/i18n/uspoof_conf.h',
      '../../deps/icu/source/i18n/uspoof_impl.cpp',
      '../../deps/icu/source/i18n/uspoof_impl.h',
      '../../deps/icu/source/i18n/uspoof_wsconf.cpp',
      '../../deps/icu/source/i18n/uspoof_wsconf.h',
    ],
    'icu_src_common_skip_54': [
      ## Strip out the following for ICU 54 only.
      ## add more conditions in the future?
      ## if your compiler can dead-strip, this will
      ## make ZERO difference to binary size.
      ## Made ICU-specific for future-proofing.
      
      # bidi- not needed (yet!)
      '../../deps/icu/source/common/ubidi.c',
      '../../deps/icu/source/common/ubidiimp.h',
      '../../deps/icu/source/common/ubidiln.c',
      '../../deps/icu/source/common/ubidiwrt.c',
      #'../../deps/icu/source/common/ubidi_props.c',
      #'../../deps/icu/source/common/ubidi_props.h',
      #'../../deps/icu/source/common/ubidi_props_data.h',
      # and the callers
      '../../deps/icu/source/common/ushape.cpp',
      '../../deps/icu/source/common/usprep.cpp',
      '../../deps/icu/source/common/uts46.cpp',
    ],
  },
}
