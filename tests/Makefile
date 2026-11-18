# Tests sur PC (Linux / macOS / WSL). Nécessite g++ et ArduinoJson 7 :
#   make ARDUINOJSON=/chemin/vers/ArduinoJson/src
FW = ../firmware/compteur_up
ARDUINOJSON ?= $(HOME)/Arduino/libraries/ArduinoJson/src
U8G2 ?= $(HOME)/Arduino/libraries/U8g2/src/clib
CXXFLAGS = -std=c++17 -O1 -Wall -Wno-unused-parameter

all: test_core test_firmware
	./test_core
	./test_firmware

test_core: test_core.cpp $(FW)/up_core.cpp $(FW)/up_core.h
	g++ $(CXXFLAGS) -I$(FW) test_core.cpp $(FW)/up_core.cpp -o $@

test_firmware: test_firmware.cpp shim/*.cpp shim/*.h $(FW)/*.cpp $(FW)/*.h
	g++ $(CXXFLAGS) -Ishim -I$(FW) -I$(ARDUINOJSON) test_firmware.cpp shim/stubs.cpp \
	  $(FW)/app.cpp $(FW)/archive.cpp $(FW)/report.cpp $(FW)/storage.cpp $(FW)/settings.cpp $(FW)/up_core.cpp -o $@

# Captures des écrans (facultatif, compile toute la bibliothèque U8g2 : ~1 min)
ecrans: test_ecrans
	./test_ecrans
	python3 ../tools/ecrans_png.py out/ecrans ../docs/ecrans.png

libu8g2.a:
	mkdir -p u8g2obj && cd u8g2obj && for f in $(U8G2)/*.c; do gcc -O1 -c -I$(U8G2) $$f; done
	ar rcs $@ u8g2obj/*.o

test_ecrans: test_ecrans.cpp libu8g2.a shim/*.cpp shim/*.h $(FW)/*.cpp $(FW)/*.h
	g++ $(CXXFLAGS) -Ishim -I$(FW) -I$(U8G2) -I$(ARDUINOJSON) test_ecrans.cpp shim/stubs.cpp $(FW)/display.cpp \
	  $(FW)/app.cpp $(FW)/archive.cpp $(FW)/report.cpp $(FW)/storage.cpp $(FW)/settings.cpp $(FW)/up_core.cpp libu8g2.a -o $@

clean:
	rm -rf test_core test_firmware test_ecrans libu8g2.a u8g2obj out
