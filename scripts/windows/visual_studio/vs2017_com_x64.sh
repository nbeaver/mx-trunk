# Visual Studio 2017 Community (64-bit) setup script for Cygwin.
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

#
# You should not have to modify anything beyond this point.
#
#----------------------------------------------------------------------

VISUAL_STUDIO_DIR="$PROGRAM_FILES_DIR\\Microsoft Visual Studio\\2017\\Community"

KIT_DIR="$PROGRAM_FILES_DIR\\Windows Kits"

SDK_DIR="$PROGRAM_FILES_DIR\\Microsoft SDKs"

DOTNET_DIR="C:\\Windows\\Microsoft.NET"

WINSDK_DIR="$SDK_DIR\\Windows\\v10.0A"

HOME_DIR="$HOMEDRIVE$HOMEPATH"

export CommandPromptType=Native

export DevEnvDir="$VISUAL_STUDIO_DIR\\Common7\\IDE\\"

export ExtensionsSdkDir="$SDK_DIR\\Windows Kits\\10\\ExtensionSDKs"

export Framework40Version=v4.0
export FrameworkDir="$DOTNET_DIR\\Framework64\\"
export FrameworkDir64="$DOTNET_DIR\\Framework64\\"
export FrameworkVersion=v4.0.30319
export FrameworkVersion64=v4.0.30319

   INCLUDE="$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\14.10.25017\\ATLMFC\\include"
   INCLUDE="$INCLUDE;$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\14.10.25017\\include"
       INCLUDE="$INCLUDE;$KIT_DIR\\NETFXSDK\\4.6.1\\include\\um"
       INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\10.0.15063.0\\ucrt"
       INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\10.0.14393.0\\shared"
       INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\10.0.14393.0\\um"
export INCLUDE="$INCLUDE;$KIT_DIR\\10\\include\\10.0.14393.0\\winrt;"

       LIB="$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\14.10.25017\\ATLMFC\\lib\\x64"
       LIB="$LIB;$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\14.10.25017\\lib\\x64"
       LIB="$LIB;$KIT_DIR\\NETFXSDK\\4.6.1\\lib\\um\\x64"
       LIB="$LIB;$KIT_DIR\\10\\lib\\10.0.15063.0\\ucrt\\x64"
export LIB="$LIB;$KIT_DIR\\10\\lib\\10.0.14393.0\\um\\x64;"

   LIBPATH="$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\14.10.25017\\ATLMFC\\lib\\x64"
   LIBPATH="$LIBPATH;$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\14.10.25017\\lib\\x64"
       LIBPATH="$LIBPATH;$KIT_DIR\\10\\UnionMetadata"
       LIBPATH="$LIBPATH;$KIT_DIR\\10\\References"
export LIBPATH="$LIBPATH;$DOTNET_DIR\\Framework64\\v4.0.30319;"

export NETFXSDKDir="KIT_DIR\\NETFXSDK\\4.6.1\\"

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

tmp=`cygpath -d -m "$WINSDK_DIR\\bin\\NETFX 4.6.1 Tools"`
cyg_netfx_dir=`cygpath -u "$tmp"`

#
# Note that the old path must be placed _after_ the new additions to the path.
# This is to ensure that the Visual Studio version of "link" is found first
# rather than the Cygwin "link".
#

old_path="$PATH"

new_path="$new_path:$cyg_visual_studio_dir/VC/Tools/MSVC/14.10.25017/bin/HostX64/x64"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE/VC/VCPackages"
new_path="$new_path:$cyg_sdk_dir/TypeScript/2.1"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE/CommonExtensions/Microsoft/TestWindow"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE/CommonExtensions/Microsoft/TeamFoundation/Team Explorer"
new_path="$new_path:$cyg_visual_studio_dir/MSBuild/15.0/bin/Roslyn"
new_path="$new_path:$cyg_performance_tools_dir"
new_path="$new_path:$cyg_short_visual_studio_dir/Shared/Common/VSPerfCollectionTools/"
new_path="$new_path:$cyg_netfx_dir"
new_path="$new_path:$cyg_kit_dir/10/bin/x64"
new_path="$new_path:$cyg_kit_dir/10/bin/10.0.14393.0/x64"
new_path="$new_path:$cyg_visual_studio_dir/MSBuild/15.0/bin"
new_path="$new_path:$cyg_fw_dir/$FrameworkVersion"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE/"
new_path="$new_path:$cyg_visual_studio_dir/Common7/Tools/"

export PATH="$new_path:$old_path"

export Platform=x64
export UCRTVersion=10.0.15063.0
export VCIDEInstallDir="$VISUAL_STUDIO_DIR\\Common7\\IDE\\VC\\"
export VCINSTALLDIR="$VISUAL_STUDIO_DIR\\VC\\"
export VCToolsInstallDir="$VISUAL_STUDIO_DIR\\VC\\Tools\\MSVC\\14.10.25017\\"
export VCToolsRedistDir="$VISUAL_STUDIO_DIR\\VC\\Redist\\MSVC\\14.10.25017\\"
export VisualStudioVersion=15.0
export VS150COMNTOOLS="$VISUAL_STUDIO_DIR\\Common7\\Tools\\"
export VSCMD_ARG_app_plat=Desktop
export VSCMD_ARG_HOST_ARCH=x64
export VSCMD_ARG_TGT_ARCH=x64
export VSCMD_VER=15.0.26403.0
export VSINSTALLDIR="$VISUAL_STUDIO_DIR\\"
export VSSDK150INSTALL="$VISUAL_STUDIO_DIR\\VSSDK"

export WindowsLibPath="$KIT_DIR\\10\\UnionMetadata;$KIT_DIR\\10\\References"
export WindowsSdkBinPath="$KIT_DIR\\bin\\"
export WindowsSdkDir="$KIT_DIR\\10\\"
export WindowsSDKLibVersion="10.0.14393.0\\"
export WindowsSdkVerBinPath="$KIT_DIR\\10\\bin\\10.0.14393.0\\"
export WindowsSDKVersion="10.0.14393.0\\"
export WindowsSDK_ExecutablePath_x64="$WINSDK_DIR\\bin\\NETFX 4.6.1 Tools\\x64\\"
export WindowsSDK_ExecutablePath_x86="$WINSDK_DIR\\bin\\NETFX 4.6.1 Tools\\"

export __DOTNET_ADD_64BIT=1
export __DOTNET_PREFERRED_BITNESS=64
export __VSCMD_PREINIT_PATH="C:\\Windows\\system32;C:\\Windows;C:\\Windows\\System32\\Wbem;C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\;$HOME_DIR\\AppData\\Local\\Microsoft\\WindowsApps;"

