#
# Visual Studio 2012 Express (x86) setup script for Cygwin.
#
# Insert this into a Cygwin startup like $HOME/.bashrc like this
#
#   if [ -f /usr/local/bin/vs2012_exp_x86.sh ]; then
#       source /usr/local/bin/vs2012_exp_x86.sh
#   fi
#

#
# You should not have to modify anything other than PROGRAM_FILES_DIR
#

PROGRAM_FILES_DIR="C:\\Program Files (x86)"

#----------------------------------------------------------------------

VISUAL_STUDIO_DIR="$PROGRAM_FILES_DIR\\Microsoft Visual Studio 11.0"

SDK_DIR="$PROGRAM_FILES_DIR\\Microsoft SDKs\\Windows"
SDK_VER=8.0

KIT_DIR="$PROGRAM_FILES_DIR\\Windows Kits\\8.0"

DOTNET_DIR="C:\\Windows\\Microsoft.NET"

export DevEnvDir="$VISUAL_STUDIO_DIR\\Common7\\IDE\\"
export ExtensionSdkDir="$SDK_DIR\\v8.0\\ExtensionSDKs"

export Framework35Version=v3.5
export FrameworkDir="$DOTNET_DIR\\Framework\\"
export FrameworkDIR32="$DOTNET_DIR\\Framework\\"
export FrameworkVersion=v4.0.30319
export FrameworkVersion32=v4.0.30319

export VCINSTALLDIR="$VISUAL_STUDIO_DIR\\VC\\"
export VisualStudioVersion=11.0
export VSINSTALLDIR="$VISUAL_STUDIO_DIR\\"
export WindowsSdkDir="$KIT_DIR\\"
export WindowsSdkDir_old="$SDK_DIR\\v8.0A\\"

       INCLUDE="$VISUAL_STUDIO_DIR\\VC\\INCLUDE"
       INCLUDE="$INCLUDE;$KIT_DIR\\include\\shared"
       INCLUDE="$INCLUDE;$KIT_DIR\\include\\um"
export INCLUDE="$INCLUDE;$KIT_DIR\\include\\winrt;"

       LIB="$VISUAL_STUDIO_DIR\\VC\\LIB"
export LIB="$LIB;$KIT_DIR\\lib\\win8\\um\\x86;"

       LIBPATH="$DOTNET_DIR\\Framework\\$FrameworkVersion"
       LIBPATH="$LIBPATH;$DOTNET_DIR\\Framework\\$Framework35Version"
       LIBPATH="$LIBPATH;$VISUAL_STUDIO_DIR\\VC\\LIB"
       LIBPATH="$LIBPATH;$KIT_DIR\\References\\CommonConfiguration\\Neutral"
export LIBPATH="$LIBPATH;$ExtensionSdkDir\\Microsoft.VCLibs\\11.0\\References\\CommonConfiguration\\neutral;"

tmp=`cygpath -d -m "$VISUAL_STUDIO_DIR"`
cyg_visual_studio_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$DOTNET_DIR"`
cyg_dotnet_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$KIT_DIR"`
cyg_kit_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$SDK_DIR"`
cyg_sdk_dir=`cygpath -u "$tmp"`

old_path="$PATH"

new_path="$cyg_visual_studio_dir/Common7/IDE/CommonExtensions/Microsoft/TestWindow"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE"
new_path="$new_path:$cyg_visual_studio_dir/VC/BIN"
new_path="$new_path:$cyg_visual_studio_dir/Common7/Tools"
new_path="$new_path:$cyg_dotnet_dir/Framework/v4.0.30319"
new_path="$new_path:$cyg_dotnet_dir/Framework/v3.5"
new_path="$new_path:$cyg_visual_studio_dir/VC/VCPackages"
new_path="$new_path:$cyg_visual_studio_dir/Team Tools/Performance Tools"
new_path="$new_path:$cyg_kit_dir/bin/x86"
new_path="$new_path:$cyg_sdk_dir/v8.0A/bin/NETFX 4.0 Tools"
new_path="$new_path:$old_path"

export PATH="$new_path"

#----------------------------------------------------------------------------
#
# Arrange for some MX-specific stuff to be created.  If you are not using MX,
# then you can delete everything from here to the end.
#

export MX_MSDEV_DIR=`cygpath -w $cyg_kit_dir/Lib/win8/um | sed 's/\\\\/\\\\\\\\/g'`

#
# The following provides a way for Windows CMD scripts to find the
# value of MX_MSDEV_DIR.  The tests for file existence should make
# sure that mx_msdev_dir.bat is only created one.
#

if [ -d /cygdrive/c/opt/mx/etc ]; then
    if [ ! -f /cygdrive/c/opt/mx/etc/mx_msdev_dir.bat ]; then
        echo "set MX_MSDEV_DIR=$MX_MSDEV_DIR"$'\r' > \
                        /cygdrive/c/opt/mx/etc/mx_msdev_dir.bat
    fi
fi

