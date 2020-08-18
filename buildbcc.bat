cd filp
call buildbcc.bat
cd ..

cd qdgdf
call buildbcc.bat
cd ..

bcc32 -c -Ifilp -Iqdgdf render.c
bcc32 -c -Ifilp -Iqdgdf map.c
bcc32 -c -Ifilp -Iqdgdf game.c
bcc32 -DVERSION=\""0.0"\" -c -Ifilp -Iqdgdf fr2002.c

bcc32 -tW -Lfilp -Lqdgdf -LC:\Borland\bcc55\lib\PSDK -efr2002.exe *.obj ddraw.lib dsound.lib filp.lib qdgdfv.lib qdgdfa.lib
