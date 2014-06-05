#!/bin/bash
# Copyright (C) 2014 IBM Corporation and others. All Rights Reserved.
#
# configure is python, rewrite this in python?

# set to 0 for 'full size' ICU data
SMALL_ICU_DATA=1
# set to 0 for 'full size' ICU code
SMALL_ICU_CODE=1

#set -x
SRCDIR="$1"
TARGDIR="$2"
FULL_ICU="$3"

if [ "$3" = "true" ];
then
    SMALL_ICU_DATA=0
fi

ROOTDIR=`pwd`
echo "$0 building from $SRCDIR to $TARGDIR"
if [ ! -d "${SRCDIR}" ];
then
    echo "WARNING: ${SRCDIR} not a directory"
    echo "Try this: "
    echo
    echo "  svn checkout --force http://source.icu-project.org/repos/icu/icu/branches/srl/10919config53 ${SRCDIR}"
    echo
    exit 1
fi

if [ ! -d "${TARGDIR}" ];
then
    mkdir -p "${TARGDIR}"
fi


CONFIGURE_OPTS="--enable-static --disable-shared --with-data-packaging=static --disable-layout --disable-tests --disable-icuio --disable-extras --disable-dyload"


# Need some way to pass these flags to ICU build AND v8.
# Could probably be done by writing out a .gyp file here to be included.?
#UCONFIG=common/unicode/uconfig.h
#SRCCONFIG="${SRCDIR}/source/${UCONFIG}"
#TARGCONFIG="${TARGDIR}/${UCONFIG}"
TARGGYPI="${TARGDIR}/icu-config.gypi"
if [ $SMALL_ICU_CODE -eq 1 ];
then
    UFLAGS="UCONFIG_NO_LEGACY_CONVERSION=1 UCONFIG_NO_IDNA=1 UCONFIG_NO_BREAK_ITERATION=0 UCONFIG_NO_TRANSLITERATION=1 UCONFIG_NO_REGULAR_EXPRESSIONS=1 UCONFIG_SRL_NO_TEST_API=1"
else
    UFLAGS=""
fi

CPPFLAGS=""
#if [ -f ${TARGCONFIG} ];
#then
#    echo "OK: ${TARGCONFIG}"
#    UPDTARGCONFIG=0
#else
#    echo "Updating ${TARGCONFIG}"
#    if [ ! -d ${TARGDIR}/common/unicode ];
#    then
#        mkdir -p ${TARGDIR}/common/unicode
#    fi
#
#    >${TARGCONFIG}
#    UPDTARGCONFIG=1
#fi

echo "Updating CPPFLAGS and ${TARGGYPI}"
cat > "${TARGGYPI}.vars" <<EOF
##
##  Add this to icu-generic.gyp under icuuc
##

                'defines': [
EOF
cat > "${TARGGYPI}" <<EOF
# Generated file from icu-generic.sh
# included by icu-generic.gyp
{
    'variables': {
EOF

for UFLAG in ${UFLAGS};
do
    FNAME=`echo ${UFLAG} | cut -d= -f1`
    FVAL=`echo ${UFLAG} | cut -d= -f2`

    CPPFLAGS="${CPPFLAGS} -D${FNAME}=${FVAL}"

cat >> "${TARGGYPI}" <<EOF
        'icu_${FNAME}':  '${FVAL}',
EOF

cat >> "${TARGGYPI}.vars" <<EOF
                    '${FNAME}=<(icu_${FNAME})',
EOF

#    if [ ${UPDTARGCONFIG} -eq 1 ];
#    then
#        echo "#define ${FNAME} ${FVAL}" >> ${TARGCONFIG}
#    fi
done

cat >> "${TARGGYPI}" <<EOF
   },
}
EOF

cat >> "${TARGGYPI}.vars" <<EOF
                ],
EOF

# if [ ${UPDTARGCONFIG} -eq 1 ];
# then
#     cat < ${SRCCONFIG} >> ${TARGCONFIG}
# fi

echo "CPPFLAGS=${CPPFLAGS}"


ORIGDIR=`pwd`

if [ -f "${TARGDIR}/config.status" ];
then
    echo "${TARGIR} already configured - assuming good.  Delete this dir to rebuild."
else
    cd "${TARGDIR}"
    ATARGDIR=`pwd`
    # v8-i18n actually uses brkiteration, though not in ES402 as of this writing.
    # UCONFIG_SRL_NO_TEST_API is experimental, http://bugs.icu-project.org/trac/ticket/10919
    env CPPFLAGS="${CPPFLAGS}" "${ORIGDIR}/${SRCDIR}/source/configure" ${CONFIGURE_OPTS} || exit 1
    cd "${ROOTDIR}"
fi

TARGRES=${TARGDIR}/data
TARGRESMK=${TARGRES}/Makefile.local

if [ $SMALL_ICU_DATA -eq 1 ];
then
    echo updating ${TARGRESMK}
    cat > "${TARGRESMK}" <<"EOF"
# -*- makefile-mode -*-
#
CORE_LOCALES:=root.txt en.txt
GENRB_SOURCE=$(CORE_LOCALES)
CURR_SOURCE=$(CORE_LOCALES)
COLLATION_SOURCE=$(CORE_LOCALES)
# want this empty
LANG_SOURCE=$(CORE_LOCALES)
# want this empty
REGION_SOURCE=$(CORE_LOCALES)
NO_UCM=1
#ALL_UCM_SOURCE=
NO_BRK=1
#ALL_BRK_SOURCE=
NO_CFU=1
ZONE_SOURCE=$(CORE_LOCALES)
NO_RBNF=1

#build a separate small ICU static lib
build-small:
	$(PKGDATA_INVOKE) $(PKGDATA) -e $(subst icu,icusm,$(ICUDATA_ENTRY_POINT)) -T $(OUTTMPDIR) -p $(ICUDATA_NAME) -m $(PKGDATA_MODE) $(PKGDATA_VERSIONING) $(subst icu,icusm,$(PKGDATA_LIBNAME)) $(PKGDATA_LIST)
EOF
else
    echo "Removing ${TARGRESMK}"
    rm -f ${TARGRESMK}
fi

echo "Building"
# CORES is set to the number of cores to use

# skip deps..
#MAKEOPTS='DEPS='
CORES=${CORES-1}

# multicore build of ICU seems broken (?)
CORES=1

echo "CORES=${CORES}"
make -j${CORES} -C "${TARGDIR}" ${MAKEOPTS} || exit 1

if [ $SMALL_ICU_DATA -eq 1 ];
then
    echo "Building small static data"
    make -C "${TARGDIR}/data" build-small

    echo "Removing the non-stub library"
    make -C "${TARGDIR}/data" cleanlib
else
    echo "Full ICU data built."
fi

echo DONE building ICU
exit 0
