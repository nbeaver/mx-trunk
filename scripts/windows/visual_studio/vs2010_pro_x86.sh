#
# Visual Studio 2010 Professional (32-bit) setup script for Cygwin.
#
# Insert this into a Cygwin startup like $HOME/.bashrc like this
#
#   if [ -f /usr/local/bin/vs2010_pro_x86.sh ]; then
#      source /usr/local/bin/vs2010_pro_x86.sh
#   fi
#

#
# You should not have to modify anything other than PROGRAM_FILES_DIR
#

PROGRAM_FILES_DIR="C:\\Program Files"

#----------------------------------------------------------------------

VISUAL_STUDIO_DIR="$PROGRAM_FILES_DIR\\Microsoft Visual Studio 10.0"

SDK_DIR="$PROGRAM_FILES_DIR\\Microsoft SDKs\\Windows\\v7.0A"

DOTNET_DIR="C:\\Windows\\Microsoft.NET"

export CommandPromptType=Native

export Framework35Version=v3.5
export FrameworkDir="$DOTNET_DIR\\Framework"
export FrameworkDIR32="$DOTNET_DIR\\Framework"
export FrameworkVersion=v4.0.30319
export FrameworkVersion32=v4.0.30319

export FSHARPINSTALLDIR="$PROGRAM_FILES_DIR\\Microsoft F#\\v4.0\\"

       INCLUDE="$VISUAL_STUDIO_DIR\\VC\\INCLUDE"
       INCLUDE="$INCLUDE;$VISUAL_STUDIO_DIR\\VC\\ATLMFC\\INCLUDE"
export INCLUDE="$INCLUDE;$SDK_DIR\\include;"

       LIB="$VISUAL_STUDIO_DIR\\VC\\LIB"
       LIB="$LIB;$VISUAL_STUDIO_DIR\\VC\\ATLMFC\\LIB"
export LIB="$LIB;$SDK_DIR\\lib;"

       LIBPATH="$FrameworkDir\\$FrameworkVersion"
       LIBPATH="$LIBPATH;$FrameworkDir\\$Framework35Version"
       LIBPATH="$LIBPATH;$VISUAL_STUDIO_DIR\\VC\\LIB"
export LIBPATH="$LIBPATH;$VISUAL_STUDIO_DIR\\VC\\ATLMFC\\LIB;"

export VCINSTALLDIR="$VISUAL_STUDIO_DIR\\VC\\"
export VSINSTALLDIR="$VISUAL_STUDIO_DIR\\"
export WindowsSdkDir="$SDK_DIR\\"

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

tmp=`cygpath -d -m "$PROGRAM_FILES_DIR\\HTML Help Workshop"`
cyg_help_dir=`cygpath -u "$tmp"`

#
# Note that the old path must be placed _after_ the new additions to the path.
# This is to ensure that the Visual Studio version of "link" is found first
# rather than the Cygwin "link".
#

old_path="$PATH"

new_path="$cyg_visual_studio_dir/VC/BIN"
new_path="$new_path:$cyg_sdk_dir/Bin"
new_path="$new_path:$cyg_fw_dir/$FrameworkVersion"
new_path="$new_path:$cyg_fw_dir/$Framework35Version"
new_path="$new_path:$cyg_visual_studio_dir/VC/VCPackages"
new_path="$new_path:$cyg_visual_studio_dir/Common7/IDE"
new_path="$new_path:$cyg_visual_studio_dir/Common7/Tools"
new_path="$new_path:$cyg_help_dir"
new_path="$new_path:$old_path"

export PATH="$new_path"

#----------------------------------------------------------------------------
#
# Arrange for some MX-specific stuff to be created.  If you are not using MX,
# then you can delete everything from here to the end.
#

export MX_MSDEV_DIR=`cygpath -w $cyg_sdk_dir | sed 's/\\\\/\\\\\\\\/g'`

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

