echo off 
echo build jpeg-turbo by MinGW
where gcc
if errorlevel 1 (
	echo MinGW/gcc NOT FOUND.
	exit -1
)
echo MinGW/gcc found.
where cmake
if errorlevel 1 (
	echo cmake NOT FOUND.
	echo download from http://cmake.org/ ,extract to disk 
	echo add installation path to environment variable PATH
	pause
	exit -1
)
echo cmake found.
where nasm
if errorlevel 1 (
	echo nasm NOT FOUND.
	echo download from https://www.nasm.us/ ,extract to disk 
	echo add installation path to environment variable PATH, version above 2.13.03 required.
	pause
	exit -1
)
echo nasm found.
set sh_folder=%~sdp0

pushd %sh_folder%\libjpeg-turbo-1.4.2
gcc --version |findstr "sjlj seh"
rem �ж��Ƿ��ܱ���64λ����
if errorlevel 1 (
	echo unsupported x86_64 build
	)else call:gcc_x86_64
	
gcc --version |findstr "sjlj dwarf"
rem �ж��Ƿ��ܱ���32λ����
if errorlevel 1 (
	echo unsupported x86 build	
	)else call:gcc_x86

goto :end
:gcc_x86
echo build x86 use MinGW 
if exist build_gcc_x86 rmdir build_gcc_x86 /s/q
mkdir build_gcc_x86
pushd build_gcc_x86
rem gcc SJLJ or DWARF distribution required

cmake -G "MinGW Makefiles" ^
	-DCMAKE_BUILD_TYPE=RELEASE ^
	-DCMAKE_C_FLAGS=-m32 ^
	-DCMAKE_INSTALL_PREFIX=%sh_folder%/release/libjpeg-turbo-windows-gcc-x86 ^
	..
make install -j8
popd
rmdir build_gcc_x86 /s/q
goto:eof

:gcc_x86_64
echo build x86_64 use MinGW 
if exist build_gcc_x86_64 rmdir build_gcc_x86_64 /s/q
mkdir build_gcc_x86_64
pushd build_gcc_x86_64
rem gcc SJLJ or SEH distribution required

cmake -G "MinGW Makefiles" ^
	-DCMAKE_BUILD_TYPE=RELEASE ^
	-DCMAKE_C_FLAGS=-m64 ^
	-DCMAKE_INSTALL_PREFIX=%sh_folder%/release/libjpeg-turbo-windows-gcc-x86_64 ^
	..
make install -j8
popd
rmdir build_gcc_x86_64 /s/q
goto:eof

:end
popd