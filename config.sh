#!/bin/sh

# Configuration shell script

TARGET=fr2002

# gets program version
VERSION=`cut -f2 -d\" VERSION`

# default installation prefix
PREFIX=/usr/local

# store command line args for configuring the libraries
CONF_ARGS="$*"

MINGW32_PREFIX=x86_64-w64-mingw32

# parse arguments
while [ $# -gt 0 ] ; do

    case $1 in
    --help)         CONFIG_HELP=1 ;;

    --mingw32-prefix=*)     MINGW32_PREFIX=`echo $1 | sed -e 's/--mingw32-prefix=//'`
                            ;;

    --mingw32)          CC=${MINGW32_PREFIX}-gcc
                        WINDRES=${MINGW32_PREFIX}-windres
                        AR=${MINGW32_PREFIX}-ar
                        LD=${MINGW32_PREFIX}-ld
                        CPP=${MINGW32_PREFIX}-g++
                        ;;

    --prefix)       PREFIX=$2 ; shift ;;
    --prefix=*)     PREFIX=`echo $1 | sed -e 's/--prefix=//'` ;;
    esac

    shift
done

if [ "$CONFIG_HELP" = "1" ] ; then

    echo "Available options:"
    echo "--prefix=PREFIX       Installation prefix ($PREFIX)."
    echo "--mingw32             Build using the mingw32 compiler."

    echo
    echo "Environment variables:"
    echo "CC                    C Compiler."
    echo "CFLAGS                Compile flags (i.e., -O3)."

    exit 1
fi

echo "Configuring Freaks 2.002..."

echo "/* automatically created by config.sh - do not modify */" > config.h
echo "# automatically created by config.sh - do not modify" > makefile.opts
> config.ldflags
> config.cflags
> .config.log

# set compiler
if [ "$CC" = "" ] ; then
    CC=cc
    # if CC is unset, try if gcc is available
    which gcc > /dev/null

    if [ $? = 0 ] ; then
        CC=gcc
    fi
fi

if [ "$LD" = "" ] ; then
    LD=ld
fi

if [ "$TAR" = "" ] ; then
    TAR=tar
fi

echo "CC=$CC" >> makefile.opts
echo "LD=$LD" >> makefile.opts
echo "TAR=$TAR" >> makefile.opts

# set cflags
if [ "$CFLAGS" = "" ] ; then
    CFLAGS="-g -Wall"
fi

echo "CFLAGS=$CFLAGS" >> makefile.opts

# Add CFLAGS to CC
CC="$CC $CFLAGS"

# add version
cat VERSION >> config.h

# add installation prefix
echo "#define CONFOPT_PREFIX \"$PREFIX\"" >> config.h

######################################################

# filp
echo -n "Looking for filp... "

for FILP in ./filp ../filp NOTFOUND ; do
    if [ -d $FILP ] && [ -f $FILP/filp.h ] ; then
        break
    fi
done

if [ "$FILP" != "NOTFOUND" ] ; then
    echo "-I$FILP" >> config.cflags
    echo "-L$FILP -lfilp" >> config.ldflags
    echo "OK ($FILP)"
else
    echo "No"
    exit 1
fi

if [ ! -f $FILP/Makefile ] ; then
    (echo ; cd $FILP ; ./config.sh $CONF_ARGS ; echo) || exit 1
fi

cat ${FILP}/config.ldflags >> config.ldflags

# qdgdf
echo -n "Looking for QDGDF... "

for QDGDF in ./qdgdf ../qdgdf NOTFOUND ; do
    if [ -d $QDGDF ] && [ -f $QDGDF/qdgdf_video.h ] ; then
        break
    fi
done

if [ "$QDGDF" != "NOTFOUND" ] ; then
    echo "-I$QDGDF" >> config.cflags
    echo "-L$QDGDF -lqdgdf" >> config.ldflags
    echo "OK ($QDGDF)"
else
    echo "No"
    exit 1
fi

if [ ! -f $QDGDF/Makefile ] ; then
    (echo ; cd $QDGDF ; ./config.sh $CONF_ARGS --without-opengl) || exit 1
fi

grep CONFOPT $QDGDF/config.h >> config.h

cat ${QDGDF}/config.ldflags >> config.ldflags

# if qdgdf includes the DirectDraw driver,
# a win32 executable is assumed
grep CONFOPT_DDRAW ${QDGDF}/config.h >/dev/null && TARGET=fr2002.exe

#########################################################

# final setup

echo "FILP=$FILP" >> makefile.opts
echo "QDGDF=$QDGDF" >> makefile.opts
echo "TARGET=$TARGET" >> makefile.opts
echo "VERSION=$VERSION" >> makefile.opts
echo "PREFIX=\$(DESTDIR)$PREFIX" >> makefile.opts
echo >> makefile.opts

cat makefile.opts makefile.in makefile.depend > Makefile

#########################################################

echo "Type 'make' to build Freaks 2.002."

# cleanup
rm -f .tmp.c .tmp.o

exit 0
