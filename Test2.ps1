cargo build --release | Out-Null
linkle nso .\target\aarch64-none-elf\release\oss-rtld.elf C:\Users\Mary\AppData\Roaming\Ryujinx\mods\contents\0100000000010000\exefs\rtld  | Out-Null
C:\Users\Mary\Downloads\ryujinx-1.0.7134-win_x64\publish\Ryujinx.exe "D:\Games\Switch\Carts\Super Mario Odyssey\game.xci"
