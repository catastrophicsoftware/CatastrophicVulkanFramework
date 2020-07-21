
for /r %%i in (*.frag, *.vert, *.comp) do glslc %%i -o %%~ni.spv