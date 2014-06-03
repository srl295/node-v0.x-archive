// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "node_i18n.h"

// TODO: IFDEF i18n..

#include <unicode/udata.h>

#define DEBUG_ICU_UTIL 0

#ifndef V8_NO_SMALL_ENTRY_POINT
/* compare following to utypes.h defs for U_ICUDATA_ENTRY_POINT */
#define SMALL_ICUDATA_ENTRY_POINT SMALL_DEF2(U_ICU_VERSION_MAJOR_NUM, U_LIB_SUFFIX_C_NAME)
#define SMALL_DEF2(major, suff) SMALL_DEF(major,suff)
#ifndef U_LIB_SUFFIX_C_NAME
#define SMALL_DEF(major, suff) icusmdt##major##_dat
#else
#define SMALL_DEF(major, suff) icusmdt##suff##major##_dat
#endif

extern "C" const char U_IMPORT SMALL_ICUDATA_ENTRY_POINT[];
#endif


namespace node {
  namespace i18n {

bool InitializeICUDirectory(const char* icu_data_path) {
#if !defined(V8_I18N_SUPPORT)
  return true;
#else
  if(icu_data_path != NULL) {
    u_setDataDirectory(icu_data_path);
#if DEBUG_ICU_UTIL
    puts("DATA DIR:");
    puts(icu_data_path);
#endif
    return true; // no error
    // TODO: Could verify that /res_index can load?
  } else {
    UErrorCode status = U_ZERO_ERROR;
#ifndef V8_NO_SMALL_ENTRY_POINT
    // install the 'small' data.
    udata_setCommonData(&SMALL_ICUDATA_ENTRY_POINT, &status);
#if DEBUG_ICU_UTIL
    puts("SMALL DATA:");
    puts(u_errorName(status));
#endif
#endif
    return (status == U_ZERO_ERROR);
  }
#endif
}
    
  } // namespace i18n 
} // namespace node
