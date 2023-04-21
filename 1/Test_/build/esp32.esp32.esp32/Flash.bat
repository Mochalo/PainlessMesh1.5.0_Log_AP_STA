set path=C:\Arduino\Test_\build\esp32.esp32.esp32\
set name=Test_
set mem=4
set copycom="%path%%name%.ino.bin"


del "C:\inetpub\wwwroot\*.bin"
copy %copycom%  "C:\inetpub\wwwroot"


set loop=1
:loop

set /a loop=%loop%+1 
if "%loop%"=="20" goto next


set command=%path%esptool.exe --chip esp32 --port COM%loop% --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size %mem%MB 0x1000 %path%%name%.ino.bootloader.bin 0x8000 %path%%name%.ino.partitions.bin  0xe000  %path%boot_app0.bin 0x10000 %path%%name%.ino.bin 
start  %command%
rem echo  command
rem pause
goto loop

:next
