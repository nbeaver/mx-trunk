$ !
$ ! This script is used to build the MX feature test programs under VMS.
$ !
$ ! The one and only argument to the script should be the top level directory
$ ! of your MX source tree.  For example, if your MX source tree was unpacked
$ ! in [users.lavender.mxdev.mx], then run this command as
$ !
$ !   @[users.lavender.mxdev.mx.scripts]vms_test_features [users.lavender.mxdev.mx
$ !
$ ! Note that you _must_ leave off the trailing ']' character for this to work.
$ !
$ topdir = ''p1'
$ featuredir = "''topdir'.test.features"
$ !
$ write sys$output "Top level directory is ''topdir']."
$ !
$ compile = "cc /debug /noopt /define=("OS_VMS","DEBUG") /warnings=(disable=longextern,errors=all) /error_limit=10 /include=''topdir'.libMx]"
$ !
$ linklibraries = "''topdir'.libMx]libMx.olb/library, tcpip$rpc:tcpip$rpcxdr/library, tcpip$library:tcpip$lib/library"
$ !
$ itimerdir = "''featuredir'.itimer_test]"
$ write sys$output "Build interval timer tests in ''itimerdir'."
$ set default 'itimerdir'
$ !
$ write sys$output "Compiling ''itimerdir'itimer_oneshot.c"
$ compile itimer_oneshot.c
$ link /exe=itimer_oneshot.exe itimer_oneshot.obj, 'linklibraries'
$ !
$ write sys$output "Compiling ''itimerdir'itimer_periodic.c"
$ compile itimer_periodic.c
$ link /exe=itimer_periodic.exe itimer_periodic.obj, 'linklibraries'
$ !
$ mutexdir = "''featuredir'.mutex_test]"
$ write sys$output "Build mutex tests in ''mutexdir'"
$ set default 'mutexdir'
$ !
$ write sys$output "Compiling ''mutexdir'mutex_lock.c"
$ compile mutex_lock.c
$ link /exe=mutex_lock.exe mutex_lock.obj, 'linklibraries'
$ !
$ write sys$output "Compiling ''mutexdir'mutex_trylock.c"
$ compile mutex_trylock.c
$ link /exe=mutex_trylock.exe mutex_trylock.obj, 'linklibraries'
$ !
$ write sys$output "Compiling ''mutexdir'mutex_recursive.c"
$ compile mutex_recursive.c
$ link /exe=mutex_recursive.exe mutex_recursive.obj, 'linklibraries'
$ !
$ semaphoredir = "''featuredir'.semaphore_test]"
$ write sys$output "Build semaphore tests in ''semaphoredir'"
$ set default 'semaphoredir'
$ !
$ write sys$output "Compiling ''semaphoredir'semaphore_lock.c"
$ compile semaphore_lock.c
$ link /exe=semaphore_lock.exe semaphore_lock.obj, 'linklibraries'
$ !
$ write sys$output "Compiling ''semaphoredir'semaphore_trylock.c"
$ compile semaphore_trylock.c
$ link /exe=semaphore_trylock.exe semaphore_trylock.obj, 'linklibraries'
$ !
$ !!!write sys$output "Compiling ''semaphoredir'named_semaphore_server.c"
$ !!!compile named_semaphore_server.c
$ !!!link /exe=named_semaphore_server.exe named_semaphore_server.obj, 'linklibraries'
$ !
$ !!!write sys$output "Compiling ''semaphoredir'named_semaphore_client.c"
$ !!!compile named_semaphore_client.c
$ !!!link /exe=named_semaphore_client.exe named_semaphore_client.obj, 'linklibraries'
$ !
$ threaddir = "''featuredir'.thread_test]"
$ write sys$output "Build thread tests in ''threaddir'"
$ set default 'threaddir'
$ !
$ write sys$output "Compiling ''threaddir'thread_stop.c"
$ compile thread_stop.c
$ link /exe=thread_stop.exe thread_stop.obj, 'linklibraries'
$ !
$ write sys$output "Compiling ''threaddir'thread_kill.c"
$ compile thread_kill.c
$ link /exe=thread_kill.exe thread_kill.obj, 'linklibraries'
$ !
$ finaldir = "''featuredir']"
$ write sys$output "All programs built.  Moving to the directory ''finaldir'."
$ set default 'finaldir'
