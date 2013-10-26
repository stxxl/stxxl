rem Build and Test Script for STXXL

cd /D %BuildFolder%
cmake %SrcFolder% -G"Visual Studio 11" -DBUILD_TESTS=ON

rem Build default project
rem cmake --build .
msbuild %MSBuildLogger%\my-msbuild-log.xml stxxl.sln

rem Run test suite
rem cmake --build . --target RUN_TESTS
