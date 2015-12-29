# Visual Studio 2015 Community (64-bit) setup script for Cygwin.
#
# Insert this into a Cygwin startup like $HOME/.bashrc like this
#
# if [ x$FrameworkDir = x ]; then
#     if [ -f /usr/local/bin/vs2015_com_x64.sh ]; then
#         source /usr/local/bin/vs2015_com_x64.sh
#     fi
# fi
#

#
#

PROGRAM_FILES_DIR="C:\\Program Files (x86)"

#
# The following two variables may change when you do updates
# of Visual Studio 2015.  They are current as of 2015-12-21
# and include the changes of Update 1.
#

NETFX_VERSION=4.6.1

export UCRTVersion=10.0.10586.0
##export UCRTVersion=10.0.10240.0

#
# You should not have to modify anything beyond this point.
#
#----------------------------------------------------------------------

VISUAL_STUDIO_DIR="$PROGRAM_FILES_DIR\\Microsoft Visual Studio 14.0"

KIT_DIR="$PROGRAM_FILES_DIR\\Windows Kits"

SDK_DIR="$PROGRAM_FILES_DIR\\Microsoft SDKs\\Windows\\v10.0A"

DOTNET_DIR="C:\\WINDOWS\\Microsoft.NET"

export CommandPromptType=Native

export Framework40Version=v4.0
export FrameworkDir="$DOTNET_DIR\\Framework64"
export FrameworkDIR64="$DOTNET_DIR\\Framework64"
export FrameworkVersion=v4.0.30319

       INCLUDE="$VISUAL_STUDIO_DIR\\VC\\INCLUDE"
       INCLUDE="$INCLUDE;$VISUAL_STUDIO_DIR\\VC\\ATLMFC\\INCLUDE"
       INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\$UCRTVersion\\ucrt"
       INCLUDE="$INCLUDE;$KIT_DIR\\NETFXSDK\\$NETFX_VERSION\\include\\um"
       INCLUDE="$INCLUDE;$KIT_DIR\\8.1\\include\\\\shared"
       INCLUDE="$INCLUDE;$KIT_DIR\\8.1\\include\\\\um"
export INCLUDE="$INCLUDE;$KIT_DIR\\8.1\\include\\\\winrt;"

       LIB="$VISUAL_STUDIO_DIR\\VC\\LIB\\amd64"
       LIB="$LIB;$VISUAL_STUDIO_DIR\\VC\\ATLMFC\\LIB\\amd64"
       LIB="$LIB;$KIT_DIR\\10\\lib\\$UCRTVersion\\ucrt\\x64"
       LIB="$LIB;$KIT_DIR\\NETFXSDK\\$NETFX_VERSION\\lib\\um\\x64"
export LIB="$LIB;$KIT_DIR\\8.1\\lib\\winv6.3\\um\\x64;"

       LIBPATH="$FrameworkDir\\$FrameworkVersion"
       LIBPATH="$LIBPATH;$VISUAL_STUDIO_DIR\\VC\\LIB\\amd64"
       LIBPATH="$LIBPATH;$KIT_DIR\\8.1\\References\\CommonConfiguration\\Neutral"
export LIBPATH="$LIBPATH;\\Microsoft.VCLibs\\14.0\\References\\CommonConfiguration\\neutral;"

export NETFXSDKDir="$KIT_DIR\\NETFXSDK\\$NETFX_VERSION\\"

export Platform=X64
export UniversalCRTSdkDir="$KIT_DIR\\10\\"
export VCINSTALLDIR="$VISUAL_STUDIO_DIR\\VC\\"
export VisualStudioVersion=14.0
export VS140COMNTOOLS="$VISUAL_STUDIO_DIR\\Common7\\Tools\\"
export VSINSTALLDIR="$VISUAL_STUDIO_DIR\\"
export WindowsLibPath="$KIT_DIR\\8.1\\Reference\\CommonConfiguration\\Neutral"
export WindowsSdkDir="$KIT_DIR\\8.1\\"
export WindowsSDKLibVersion="winv6.3\\"
export WindowsSDKVersion="\\"
export WindowsSDK_ExecutablePath_x64="$SDK_DIR\\bin\\NETFX $NETFX_VERSION Tools\\x64\\"
export WindowsSDK_ExecutablePath_x86="$SDK_DIR\\bin\\NETFX $NETFX_VERSION Tools\\"

#
# The PATH variable must _not_ have any spaces in it.  This is to make it
# easier to write rules in Gnu Make.  Thus, we convert the directory names
# used by Visual Studio into Windows "short" names that have no spaces.
#

tmp=`cygpath -d -m "$VISUAL_STUDIO_DIR"`
cyg_visual_studio_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$DOTNET_DIR"`
cyg_dotnet_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$SDK_DIR"`
cyg_sdk_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$FrameworkDir"`
cyg_fw_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$KIT_DIR"`
cyg_kit_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$PROGRAM_FILES_DIR\\HTML Help Workshop"`
cyg_help_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$VISUAL_STUDIO_DIR\\Team Tools\\Performance Tools"`
cyg_teamtools_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$SDK_DIR\\bin\\NETFX $NETFX_VERSION Tools"`
cyg_netfx_dir=`cygpath -u "$tmp"`

#
# Note that the old path must be placed _after_ the new additions to the path.
# This is to ensure that the Visual Studio version of "link" is found first
# rather than the Cygwin "link".
#

old_path="$PATH"

new_path="$cyg_visual_studio_dir/Common7/IDE/CommonExtensions/Microsoft/TestWindow"
new_path="$new_path:$cyg_program_files_dir/MSBuild/14.0/bin/amd64"
new_path="$new_path:$cyg_visual_studio_dir/VC/BIN/amd64"
new_path="$new_path:$cyg_fw_dir/$FrameworkVersion"
new_path="$new_path:$cyg_visual_studio_dir/VC/VCPackages"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE"
new_path="$new_path:$cyg_visual_studio_dir/Common7/Tools"
new_path="$new_path:$cyg_help_dir"
new_path="$new_path:$cyg_teamtools_dir/x64"
new_path="$new_path:$cyg_teamtools_dir"
new_path="$new_path:$cyg_kit_dir/8.1/bin/x64"
new_path="$new_path:$cyg_kit_dir/8.1/bin/x86"
new_path="$new_path:$cyg_netfx_dir/x64"

export PATH="$new_path:$old_path"

#----------------------------------------------------------------------------
#
# Arrange for some MX-specific stuff to be created.  If you are not using MX,
# then you can delete everything from here to the end.
#

export MX_MSDEV_DIR=`cygpath -w "$cyg_kit_dir/8.1/Lib/winv6.3/um" | sed 's/\\\\/\\\\\\\\/g'`

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

