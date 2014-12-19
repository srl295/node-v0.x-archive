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


/*
 * notes: by srl295
 *  - When in NODE_HAVE_SMALL_ICU mode, ICU is linked against "stub" (null) data
 *     ( stubdata/libicudata.a ) containing nothing, no data, and it's also
 *    linked against a "small" data file which the SMALL_ICUDATA_ENTRY_POINT
 *    macro names. That's the "english+root" data.
 *
 *    If icu_data_path is non-null, the user has provided a path and we assume
 *    it goes somewhere useful. We set that path in ICU, and exit.
 *    If icu_data_path is null, they haven't set a path and we want the
 *    "english+root" data.  We call
 *       udata_setCommonData(SMALL_ICUDATA_ENTRY_POINT,...)
 *    to load up the english+root data.
 *
 *  - when NOT in NODE_HAVE_SMALL_ICU mode, ICU is linked directly with its full
 *    data. All of the variables and command line options for changing data at
 *    runtime are disabled, as they wouldn't fully override the internal data.
 *    See:  http://bugs.icu-project.org/trac/ticket/10924
 */


#include "node_i18n.h"

#if defined(NODE_HAVE_I18N_SUPPORT)

#include "uv.h"
#include <limits.h>  // PATH_MAX
#include <unicode/putil.h>
#include <unicode/udata.h>

#ifndef NODE_EXEPATH_ICUDIR
#define NODE_EXEPATH_ICUDIR "../share/node/icu/"
// TODO: if  windows, mac, ...
#endif

#ifdef NODE_HAVE_SMALL_ICU
/* if this is defined, we have a 'secondary' entry point.
   compare following to utypes.h defs for U_ICUDATA_ENTRY_POINT */
#define SMALL_ICUDATA_ENTRY_POINT \
  SMALL_DEF2(U_ICU_VERSION_MAJOR_NUM, U_LIB_SUFFIX_C_NAME)
#define SMALL_DEF2(major, suff) SMALL_DEF(major, suff)
#ifndef U_LIB_SUFFIX_C_NAME
#define SMALL_DEF(major, suff) icusmdt##major##_dat
#else
#define SMALL_DEF(major, suff) icusmdt##suff##major##_dat
#endif

extern "C" const char U_DATA_API SMALL_ICUDATA_ENTRY_POINT[];
#endif

namespace node {
namespace i18n {

/**
 * Modify 'path' to remove the final (leaf) entry.
 *   so /path/to/something  -> /path/to
 *  and /path/to/dir/   -> /path/to/dir
 */
static char* my_dirname(char* path) {
  char *p = strrchr(path, U_FILE_SEP_CHAR);
#if ( (U_FILE_SEP_CHAR) != (U_FILE_ALT_SEP_CHAR) )
  // windows: use '/' if further out than '\'
  char *p2 = strrchr(path, U_FILE_ALT_SEP_CHAR);
  if(p && p2 && (p2>p)) p = p2;
#endif
  if(p) {
    *p = 0;
  }
  return path;
}

#if NODE_HAVE_SMALL_ICU
/**
  * @return true if path is a directory
  */
static bool my_isdir(const char* path) {
  int r;
  uv_fs_t req;
  r = uv_fs_stat(uv_default_loop(), &req, path, NULL);
  bool ret = ((r == 0) && (req.result == 0) && (req.statbuf.st_mode & S_IFDIR));
  printf("my_isdir %s = %c\n", path, (ret)?'T':'f');
  printf("r: %d, result: %d, flags: %x\n", r, req.result, req.statbuf.st_flags);
  //printf("#: %x\n", ((uv_stat_t*)req.ptr)->st_mode);
  puts(uv_strerror(r));
  uv_fs_req_cleanup(&req);
  return ret;
}


#endif

bool InitializeICUDirectory(const char* icu_data_path) {
  if (icu_data_path != NULL) {
    u_setDataDirectory(icu_data_path);
    return true;  // no error
  } else {
    UErrorCode status = U_ZERO_ERROR;
#ifdef NODE_HAVE_SMALL_ICU
    bool doSetCommonData = true; //< true iff we should call setCommonData
#endif
    // get the exe path
    size_t exec_path_len = 2 * PATH_MAX;
    char* exec_path = new char[exec_path_len];
    char* exec_subpath = new char[exec_path_len];
    if (uv_exepath(exec_path, &exec_path_len) == 0) {
      // TODO strNcat!
      ::strcpy(exec_subpath, exec_path);
      my_dirname(exec_subpath); // trim off '/node'
      ::strcat(exec_subpath, U_FILE_SEP_STRING); // "/"-ish
      ::strcat(exec_subpath, NODE_EXEPATH_ICUDIR);

      // always set the path. May or may not be extant
      u_setDataDirectory(exec_subpath);

#ifdef NODE_HAVE_SMALL_ICU
      // in the small-icu case, we should check whether this
      // directory actually exists.
      if (my_isdir(exec_subpath)) {
          puts("skipping setcommondata");
          doSetCommonData = false;
      }
#endif
    }
    delete [] exec_path;
    delete [] exec_subpath;

#ifdef NODE_HAVE_SMALL_ICU
    // install the 'small' data.
    if (doSetCommonData) {
      udata_setCommonData(&SMALL_ICUDATA_ENTRY_POINT, &status);
    }
#else  // !NODE_HAVE_SMALL_ICU
    // no small data, so nothing to do.
#endif  // !NODE_HAVE_SMALL_ICU
    return (status == U_ZERO_ERROR);
  }
}

}  // namespace i18n
}  // namespace node

#endif  // NODE_HAVE_I18N_SUPPORT
