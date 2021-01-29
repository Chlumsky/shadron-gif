
all:
	g++ -dynamiclib -std=c++11 -O2 -I. -lgif -limagequant src/*.cpp -o shadron-gif.dylib

static:
	g++ -dynamiclib -std=c++11 -O2 -I. /usr/local/lib/libgif.a /usr/local/lib/libimagequant.a src/*.cpp -o shadron-gif.dylib

install:
	mkdir -p ~/.config/Shadron/extensions
	cp -f shadron-gif.dylib ~/.config/Shadron/extensions/shadron-gif.dylib

clean:
	rm -f shadron-gif.dylib
