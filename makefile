
rigctl: rigctl.cpp rigctl.h
	gcc -o rig rigctl.cpp -fpic -I/usr/include/x86_64-linux-gnu/qt5 -lc++ -lQt5Core -lQt5Network
