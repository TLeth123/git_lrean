
这是一个基于littlefs文件系统的虚拟出了一个内存为1M的存储空间（./out/usmartgo_lfs_mirror.bin）,这个点bin可以直接烧写进入硬件里面  
还带有cJSON的解析  
把想要写入内存的.bin文件放入files文件夹中，执行.exe文件就可以把files问价里面的.bin文件写入虚拟内存usmartgo_lfs_mirror.bin文件中

