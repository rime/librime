rem this generates opencc.def and opencc.lib for linking to opencc.dll in msvc
rem mingw and msvc command line tools are needed
rem you can install gendef with: mingw-get install gendef
gendef opencc.dll - > opencc.def
lib /machine:ix86 /def:opencc.def
