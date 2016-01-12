#
# Cygwin startup script for Visual C++ 5.0 support on 32-bit computers.
#

MSDEV_DIR="C:\\Program Files\\DevStudio"

#---------------------------------------------------------------------------

tmp=`cygpath -d -m "$MSDEV_DIR"`

cyg_msdev_dir=`cygpath -u "$tmp"`

cyg_msdev_win_dir=`cygpath -w "$tmp"`

#
# Different paths are used for Windows 9x and Windows NT installations
#

if [ x${OS} = "xWindows_NT" ]; then
    export PATH="${cyg_msdev_dir}/SharedIDE/bin:${cyg_msdev_dir}/VC/bin:${cyg_msdev_dir}/VC/bin/WINNT:${PATH}"
else
    export PATH="${cyg_msdev_dir}/SharedIDE/bin:${cyg_msdev_dir}/VC/bin:${cyg_msdev_dir}/VC/bin/win95:${PATH}"
fi

export INCLUDE="${cyg_msdev_win_dir}\\VC\\include;${cyg_msdev_win_dir}\\VC\\mfc\\include;${cyg_msdev_win_dir}\\VC\\atl\\include;${INCLUDE}"

export LIB="${cyg_msdev_win_dir}\\VC\\lib;${cyg_msdev_win_dir}\\VC\\mfc\\lib;${LIB}"

#---------------------------------------------------------------------------

export MX_MSDEV_DIR=`echo ${cyg_msdev_win_dir}\\\\VC | sed 's,\\\\,\\\\\\\\,g'`

