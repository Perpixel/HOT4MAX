pushd ..\

set SdkVersion=2018
set Toolset=v141
set MSBUILD_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin"

set MaxSdkDir=..\3rdparty\Autodesk\%SdkVersion%
set Dependencies=comctl32.lib;mnmath.lib;paramblk2.lib;core.lib;geom.lib;gfx.lib;maxutil.lib;poly.lib;mesh.lib;bmm.lib;assetmanagement.lib;blitz.lib;libfftw3f-3.lib

%MSBUILD_PATH%\MSBuild.exe .\vs2012\hotOcean_Max.vcxproj /m /p:Configuration=Release;Platform=x64

pause

del Hot4Max_%SdkVersion%_64bit.zip
.\3rdparty\7z\7z a -tzip .\Release\Hot4Max_%SdkVersion%_64bit.zip readme.html
.\3rdparty\7z\7z a -tzip .\Release\Hot4Max_%SdkVersion%_64bit.zip .\3rdparty\win64\libfftw3f-3.dll
.\3rdparty\7z\7z a -tzip .\Release\Hot4Max_%SdkVersion%_64bit.zip .\plugins\%SdkVersion%\x64\hotOcean_Max_%SdkVersion%.dlm

popd