
for /r %%i in (*.frag, *.vert) do glslc %%i -o %%~ni.spv