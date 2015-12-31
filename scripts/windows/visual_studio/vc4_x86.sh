#
# Cygwin startup script for Visual C++ 4.0 support on 32-bit computers.
#

MSDEV_DIR="C:\\MSDEVSTD"

#---------------------------------------------------------------------------

tmp=`cygpath -d -m "$MSDEV_DIR"`
cyg_msdev_dir=`cygpath -u "$tmp"`

if [ x${OS} = "xWindows_NT" ]; then
    export PATH="${cyg_msdev_dir}/BIN:${cyg_msdev_dir}/BIN/WINNT:${PATH}"
else
    export PATH="${cyg_msdev_dir}/BIN:${cyg_msdev_dir}/BIN/WIN95:${PATH}"
fi

export INCLUDE="${MSDEV_DIR}\\INCLUDE;${MSDEV_DIR}\\MFC\\INCLUDE;${INCLUDE}"
export LIB="${MSDEV_DIR}\\LIB;${MSDEV_DIR}\\MFC\\LIB;${LIB}"

#---------------------------------------------------------------------------

export MX_MSDEV_DIR=`echo ${MSDEV_DIR} | sed 's,\\\\,\\\\\\\\,g'`

