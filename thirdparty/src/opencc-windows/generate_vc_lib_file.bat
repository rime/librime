rem this generates opencc.def and opencc.lib for linking to opencc.dll in msvc
rem mingw and msvc command line tools are needed here
pexports opencc.dll -o > opencc.def
lib /machine:ix86 /def:opencc.def
