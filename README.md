Evented I/O for V8 javascript.
===

### To build:

Prerequisites (Unix only):

    * GCC 4.2 or newer
    * Python 2.6 or 2.7
    * GNU Make 3.81 or newer
    * libexecinfo (FreeBSD and OpenBSD only)

Unix/Macintosh:

    ./configure
    make
    make install

With ICU i18n support
( Chromium's ICU: older rev, missing some locales?, larger output size )

    svn checkout --force --revision 214189 \
        http://src.chromium.org/svn/trunk/deps/third_party/icu46 \
        deps/v8/third_party/icu46
    ./configure --with-icu-path=deps/v8/third_party/icu46/icu.gyp
    make
    make install

EXPERIMENTAL: alternate ICU support

    svn checkout --force \
     http://source.icu-project.org/repos/icu/icu/branches/srl/10919config53 \
     deps/icu
    ./configure --with-generic-icu=deps/icu
    make
    make install


   * Some rough edges on Windows.
      * for windows, add `small-icu` or `full-icu` to vcbuild params. (WIP)
      * As of this writing only `full-icu` will build, BUT will require
      setting `NODE_ICU_DATA` or using `--icu-data-dir` to function. (WIP.)
   * Builds a restricted ICU set
      * English and Root data only
      * Only the services needed by v8's Intl implementation
   * Specify an additional icu data file with either:
      * The `NODE_ICU_DATA` env variable:   `env NODE_ICU_DATA=/some/path node`
      * The `--icu-data-dir` parameter:   `node --icu-data-dir=/some/path`
   * Example:  If you use the path `/some/path`, then ICU 53 on Little
     Endian (l) finds:
      * individual files such as `/some/path/icudt53l/root.res`
      * a packaged data file `/some/path/icudt53l.dat`
   * Notes:
      * See `u_setDataDirectory()` and
        [the ICU Users Guide](http://userguide.icu-project.org/icudata)
        for many more details.
      * "53l" will be "53b" on a big endian machine.
      * To get a "full" icudt53*.dat, goto http://site.icu-project.org/download
          * for icudt53l.dat: it is present in the ICU source .tgz
          * for icudt53b.dat: download source and build ICU
      * the additional configure option `--with-full-icu` will also
        build a full ICU, this causes the env var and param mentioned
        above to become no-ops.
   * TODO:
      * instead of svn checkout, allow using a 'stock' ICU from
        http://site.icu-project.org/download with larger output
      * switch to build 'all data' by default.


If your python binary is in a non-standard location or has a
non-standard name, run the following instead:

    export PYTHON=/path/to/python
    $PYTHON ./configure
    make
    make install

Prerequisites (Windows only):

    * Python 2.6 or 2.7
    * Visual Studio 2010 or 2012

Windows:

    vcbuild nosign

You can download pre-built binaries for various operating systems from
[http://nodejs.org/download/](http://nodejs.org/download/).  The Windows
and OS X installers will prompt you for the location to install to.
The tarballs are self-contained; you can extract them to a local directory
with:

    tar xzf /path/to/node-<version>-<platform>-<arch>.tar.gz

Or system-wide with:

    cd /usr/local && tar --strip-components 1 -xzf \
                         /path/to/node-<version>-<platform>-<arch>.tar.gz

### To run the tests:

Unix/Macintosh:

    make test

Windows:

    vcbuild test

### To build the documentation:

    make doc

### To read the documentation:

    man doc/node.1

Resources for Newcomers
---
  - [The Wiki](https://github.com/joyent/node/wiki)
  - [nodejs.org](http://nodejs.org/)
  - [how to install node.js and npm (node package manager)](http://www.joyent.com/blog/installing-node-and-npm/)
  - [list of modules](https://github.com/joyent/node/wiki/modules)
  - [searching the npm registry](http://npmjs.org/)
  - [list of companies and projects using node](https://github.com/joyent/node/wiki/Projects,-Applications,-and-Companies-Using-Node)
  - [node.js mailing list](http://groups.google.com/group/nodejs)
  - irc chatroom, [#node.js on freenode.net](http://webchat.freenode.net?channels=node.js&uio=d4)
  - [community](https://github.com/joyent/node/wiki/Community)
  - [contributing](https://github.com/joyent/node/wiki/Contributing)
  - [big list of all the helpful wiki pages](https://github.com/joyent/node/wiki/_pages)
