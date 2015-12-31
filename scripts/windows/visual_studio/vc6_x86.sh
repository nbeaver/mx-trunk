#
# Cygwin startup script for Visual C++ 6.0 support on 32-bit computers.
#

MSDEV_DIR="C:\\Program Files\\Microsoft Visual Studio"

#---------------------------------------------------------------------------

tmp=`cygpath -d -m "$MSDEV_DIR"`

cyg_msdev_dir=`cygpath -u "$tmp"`

cyg_msdev_win_dir=`cygpath -w "$tmp"`

#
# Different paths are used for Windows 9x and Windows NT installations.
#

if [ x${OS} = "xWindows_NT" ]; then
    export PATH="${MSDEV_DIR}/Common/MSDev98/Bin:${MSDEV_DIR}/VC98/Bin:${MSDEV_DIR}/Common/TOOLS/WINNT:${MSDEV_DIR}/Common/TOOLS:${PATH}"
else
    export PATH="${MSDEV_DIR}/Common/MSDev98/Bin:${MSDEV_DIR}/VC98/Bin:${MSDEV_DIR}/Common/TOOLS/Win95:${MSDEV_DIR}/Common/TOOLS:${WINDIR}/SYSTEM:${PATH}"
fi

export INCLUDE="${cyg_msdev_win_dir}\\VC98\\ATL\\Include;${cyg_msdev_win_dir}\\VC98\\Include;${cyg_msdev_win_dir}\\VC98\\MFC\\Include;${INCLUDE}"

export LIB="${cyg_msdev_win_dir}\\VC98\\Lib;${cyg_msdev_win_dir}\\VC98\\MFC\\Lib;${LIB}"

#---------------------------------------------------------------------------

export MX_MSDEV_DIR=`echo ${cyg_msdev_win_dir}\\\\VC98 | sed 's,\\\\,\\\\\\\\,g'`

