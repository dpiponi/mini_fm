mini_fm: mini_fm.cpp
	#g++ -O3 -o mini_fm mini_fm.cpp -I /opt/local/include/libusb-1.0 -L /opt/local/lib -lrtlsdr
	g++ -O3 -o mini_fm mini_fm.cpp -I/usr/local/Cellar/librtlsdr/0.6.0/include/ -L/usr/local/Cellar/librtlsdr/0.6.0/lib/ -lrtlsdr
