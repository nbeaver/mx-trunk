# Visual Studio 2019 Community (64-bit) setup script for Cygwin.
#
# Insert this into a Cygwin startup like $HOME/.bashrc like this
#
# if [ x$FrameworkDir = x ]; then
#     if [ -f /usr/local/bin/vs2017_com_x64.sh ]; then
#         source /usr/local/bin/vs2017_com_x64.sh
#     fi
# fi
#

#
#

PROGRAM_FILES_DIR="C:\\Program Files (x86)"

#----
# For VS Community 2019 version 16.6.4

FRAMEWORK_MAJOR="v4.0"
FRAMEWORK_VERSION="v4.0.30319"
MSVC_VERSION="14.26.28801"
NETFX_SDK_VERSION="4.8"
TYPESCRIPT_VERSION="2.6"
VISUAL_STUDIO_VERSION="16.0"
VISUAL_STUDIO_CMD_VERSION="16.6.4"
WINDOWS_SDK_MAJOR="v10.0A"
WINDOWS_SDK_VERSION="10.0.18362.0"

##----
## For VS Community 2019 version 16.3.5
#
#FRAMEWORK_MAJOR="v4.0"
#FRAMEWORK_VERSION="v4.0.30319"
#MSVC_VERSION="14.23.28105"
#NETFX_SDK_VERSION="4.8"
#TYPESCRIPT_VERSION="2.6"
#VISUAL_STUDIO_VERSION="16.0"
#VISUAL_STUDIO_CMD_VERSION="16.3.5"
#WINDOWS_SDK_MAJOR="v10.0A"
#WINDOWS_SDK_VERSION="10.0.18362.0"

##----
## For VS Community 2017 version 15.8.1
#
#FRAMEWORK_MAJOR="v4.0"
#FRAMEWORK_VERSION="v4.0.30319"
#MSVC_VERSION="14.15.26726"
#NETFX_SDK_VERSION="4.6.1"
#TYPESCRIPT_VERSION="2.6"
#VISUAL_STUDIO_VERSION="15.0"
#VISUAL_STUDIO_CMD_VERSION="15.0"
#WINDOWS_SDK_MAJOR="v10.0A"
#WINDOWS_SDK_VERSION="10.0.17134.0"

#
# You should not have to modify anything beyond this point.
#
#----------------------------------------------------------------------

VISUAL_STUDIO_DIR="$PROGRAM_FILES_DIR\\Microsoft Visual Studio\\2019\\Community"

KIT_DIR="$PROGRAM_FILES_DIR\\Windows Kits"

SDK_DIR="$PROGRAM_FILES_DIR\\Microsoft SDKs"

DOTNET_DIR="C:\\Windows\\Microsoft.NET"

WINSDK_DIR="$SDK_DIR\\Windows\\$WINDOWS_SDK_MAJOR"

HOME_DIR="$HOMEDRIVE$HOMEPATH"

export CommandPromptType=Native

export DevEnvDir="$VISUAL_STUDIO_DIR\\Common7\\IDE\\"

export ExtensionSdkDir="$SDK_DIR\\Windows Kits\\10\\ExtensionSDKs"

export Framework40Version=$FRAMEWORK_MAJOR
export FrameworkDir="$DOTNET_DIR\\Framework64\\"
export FrameworkDir64="$DOTNET_DIR\\Framework64\\"
export FrameworkVersion=$FRAMEWORK_VERSION
export FrameworkVersion64=$FRAMEWORK_VERSION

export FSHARPINSTALLDIR="$DevEnvDir\\CommonExtensions\\Microsoft\\FSharp\\"

  INCLUDE="$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\$MSVC_VERSION\\ATLMFC\\include"
  INCLUDE="$INCLUDE;$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\$MSVC_VERSION\\include"
  INCLUDE="$INCLUDE;$KIT_DIR\\NETFXSDK\\$NETFX_SDK_VERSION\\include\\um"
  INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\$WINDOWS_SDK_VERSION\\ucrt"
  INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\$WINDOWS_SDK_VERSION\\shared"
  INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\$WINDOWS_SDK_VERSION\\um"
  INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\$WINDOWS_SDK_VERSION\\winrt;"
export INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\$WINDOWS_SDK_VERSION\\cppwinrt;"

  LIB="$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\$MSVC_VERSION\\ATLMFC\\lib\\x64"
  LIB="$LIB;$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\$MSVC_VERSION\\lib\\x64"
  LIB="$LIB;$KIT_DIR\\NETFXSDK\\$NETFX_SDK_VERSION\\lib\\um\\x64"
  LIB="$LIB;$KIT_DIR\\10\\lib\\$WINDOWS_SDK_VERSION\\ucrt\\x64"
export LIB="$LIB;$KIT_DIR\\10\\lib\\$WINDOWS_SDK_VERSION\\um\\x64;"

  LIBPATH="$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\$MSVC_VERSION\\ATLMFC\\lib\\x64"
  LIBPATH="$LIBPATH;$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\$MSVC_VERSION\\lib\\x64"
  LIBPATH="$LIBPATH;$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\$MSVC_VERSION\\lib\\x86\\store\\references"
  LIBPATH="$LIBPATH;$KIT_DIR\\10\\UnionMetadata\\$WINDOWS_SDK_VERSION"
  LIBPATH="$LIBPATH;$KIT_DIR\\10\\References\\$WINDOWS_SDK_VERSION"
export LIBPATH="$LIBPATH;$DOTNET_DIR\\Framework64\\$FRAMEWORK_VERSION;"

export NETFXSDKDir="KIT_DIR\\NETFXSDK\\$NETFX_SDK_VERSION\\"

#
# The PATH variable must _not_ have any spaces in it.  This is to make it
# easier to write rules in Gnu Make.  Thus, we convert the directory names
# used by Visual Studio into Windows "short" names that have no spaces.
#

tmp=`cygpath -d -m "$VISUAL_STUDIO_DIR"`
cyg_visual_studio_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$PROGRAM_FILES_DIR\\Microsoft Visual Studio"`
cyg_short_visual_studio_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$SDK_DIR"`
cyg_sdk_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$WINSDK_DIR"`
cyg_winsdk_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$FrameworkDir"`
cyg_fw_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$KIT_DIR"`
cyg_kit_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$VISUAL_STUDIO_DIR\\Common7\\IDE\\CommonExtensions\\Microsoft\\TeamFoundation\\Team Explorer"`
cyg_teamexplorer_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$VISUAL_STUDIO_DIR\\Team Tools\\Performance Tools"`
cyg_performance_tools_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$WINSDK_DIR\\bin\\NETFX $NETFX_SDK_VERSION Tools"`
cyg_netfx_dir=`cygpath -u "$tmp"`

tmp=`cygpath -d -m "$VISUAL_STUDIO_DIR\\Common7\\IDE\\CommonExtensions\\Microsoft\\FSharp"`
cyg_fsharp_dir=`cygpath -u "$tmp"`

#
# Note that the old path must be placed _after_ the new additions to the path.
# This is to ensure that the Visual Studio version of "link" is found first
# rather than the Cygwin "link".
#

old_path="$PATH"

new_path="$new_path:$cyg_visual_studio_dir/VC/Tools/MSVC/$MSVC_VERSION/bin/HostX64/x64"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE/VC/VCPackages"
#new_path="$new_path:$cyg_sdk_dir/TypeScript/$TYPESCRIPT_VERSION"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE/CommonExtensions/Microsoft/TestWindow"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE/CommonExtensions/Microsoft/TeamFoundation/Team Explorer"
new_path="$new_path:$cyg_visual_studio_dir/MSBuild/Current/bin/Roslyn"
new_path="$new_path:$cyg_performance_tools_dir/x64"
new_path="$new_path:$cyg_performance_tools_dir"
new_path="$new_path:$cyg_short_visual_studio_dir/Shared/Common/VSPerfCollectionTools/x64"
new_path="$new_path:$cyg_short_visual_studio_dir/Shared/Common/VSPerfCollectionTools/"
new_path="$new_path:$cyg_netfx_dir"
new_path="$new_path:$cyg_fsharp_dir"
new_path="$new_path:$cyg_kit_dir/10/bin/$WINDOWS_SDK_VERSION/x64"
new_path="$new_path:$cyg_kit_dir/10/bin/x64"
new_path="$new_path:$cyg_visual_studio_dir/MSBuild/Current/bin"
new_path="$new_path:$cyg_fw_dir/$FrameworkVersion"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE/"
new_path="$new_path:$cyg_visual_studio_dir/Common7/Tools/"

export PATH="$new_path:$old_path"

export Platform=x64
export UCRTVersion=$WINDOWS_SDK_VERSION
export UniversalCRTSdkDir=$KIT_DIR\\10\\
export VCIDEInstallDir="$VISUAL_STUDIO_DIR\\Common7\\IDE\\VC\\"
export VCINSTALLDIR="$VISUAL_STUDIO_DIR\\VC\\"
export VCToolsInstallDir="$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\$MSVC_VERSION\\"
export VCToolsRedistDir="$VISUAL_STUDIO_DIR\\VC\\Redist\\MSVC\\$MSVC_VERSION\\"
export VisualStudioVersion=$VISUAL_STUDIO_VERSION
export VS160COMNTOOLS="$VISUAL_STUDIO_DIR\\Common7\\Tools\\"
export VSCMD_ARG_app_plat=Desktop
export VSCMD_ARG_HOST_ARCH=x64
export VSCMD_ARG_TGT_ARCH=x64
export VSCMD_VER=$VISUAL_STUDIO_CMD_VERSION
export VSINSTALLDIR="$VISUAL_STUDIO_DIR\\"
export VSSDK150INSTALL="$VISUAL_STUDIO_DIR\\VSSDK"
export VSSDKINSTALL="$VISUAL_STUDIO_DIR\\VSSDK"

export WindowsLibPath="$KIT_DIR\\10\\UnionMetadata\\$WINDOWS_SDK_VERSION;$KIT_DIR\\10\\References\\$WINDOWS_SDK_VERSION"
export WindowsSdkBinPath="$KIT_DIR\\10\\bin\\"
export WindowsSdkDir="$KIT_DIR\\10\\"
export WindowsSDKLibVersion="$WINDOWS_SDK_VERSION\\"
export WindowsSdkVerBinPath="$KIT_DIR\\10\\bin\\$WINDOWS_SDK_VERSION\\"
export WindowsSDKVersion="$WINDOWS_SDK_VERSION\\"
export WindowsSDK_ExecutablePath_x64="$WINSDK_DIR\\bin\\NETFX $NETFX_SDK_VERSION Tools\\x64\\"
export WindowsSDK_ExecutablePath_x86="$WINSDK_DIR\\bin\\NETFX $NETFX_SDK_VERSION Tools\\"

export __DOTNET_ADD_64BIT=1
export __DOTNET_PREFERRED_BITNESS=64
export __VSCMD_PREINIT_PATH="C:\\Windows\\system32;C:\\Windows;C:\\Windows\\System32\\Wbem;C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\;$HOME_DIR\\AppData\\Local\\Microsoft\\WindowsApps;"

#----------------------------------------------------------------------------
#
# Arrange for some MX-specific stuff to be created.  If you are not using MX,
# then you can delete everything from here to the end.
#

export MX_MSDEV_DIR=`cygpath -w "$cyg_kit_dir/10/Lib/$WINDOWS_SDK_VERSION/um" | sed 's/\\\\/\\\\\\\\/g'`

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

