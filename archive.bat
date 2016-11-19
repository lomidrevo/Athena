
set archiveFolder="c:\filip\Dropbox\programming\AllTheCOde\archive\"

rem remove project output directories
rmdir "code\.AllTheCode-Release-x64" /q /s
rmdir "code\.AllTheCode-Debug-x64" /q /s
rmdir "code\.TestLibrary-Release-x64" /q /s
rmdir "code\.TestLibrary-Debug-x64" /q /s
rmdir "code\.Athena.Cuda-Release-x64" /q /s
rmdir "code\.Athena.Cuda-Debug-x64" /q /s

rmdir "code\AllTheCode\.Release-x64" /q /s
rmdir "code\AllTheCode\.Debug-x64" /q /s
rmdir "code\TestLibrary\.Release-x64" /q /s
rmdir "code\TestLibrary\.Debug-x64" /q /s
rmdir "code\Athena.Cuda\.Release-x64" /q /s
rmdir "code\Athena.Cuda\.Debug-x64" /q /s

del "code\*.sdf"
del "code\AllTheCode\*.log"

rem archive to .rar
rar.exe a -r -m5 -agYYYY-MM-DD-HH-MM "AllTheCode..rar" "."

rem copy to archive
move "*.rar" %archiveFolder%