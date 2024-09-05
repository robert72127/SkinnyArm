/*
 TODO

 open adds fd number to fd list of process,
 close removes it

 since we only use rootfs
 process fd will map to one of always open files there

 read: if fd = 0 then read from uart else read from actuall file 
 kernel read address of bufer,
 walk proces memory map, find place
 and copies data there

 write only for fd 0, same way with addres walk
*/