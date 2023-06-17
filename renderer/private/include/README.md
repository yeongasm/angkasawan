## Private Includes Folder

Header files that are only included in .cpp files should be placed in this folder. If any of the header files in here are included in other other files which is then accessed publicly, should be placed in `public/renderer`.

> Files in here should never have public access.