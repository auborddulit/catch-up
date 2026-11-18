# Tests sur PC (Linux / macOS / WSL). Nécessite g++ et ArduinoJson 7 :
#   make ARDUINOJSON=/chemin/vers/ArduinoJson/src
FW = ../firmware/compteur_up
ARDUINOJSON ?= $(HOME)/Arduino/libraries/ArduinoJson/src
CXXFLAGS = -std=c++17 -O1 -Wall -Wno-unused-parameter

all: test_core test_firmware
	./test_core
	./test_firmware

test_core: test_core.cpp $(FW)/up_core.cpp $(FW)/up_core.h
	g++ $(CXXFLAGS) -I$(FW) test_core.cpp $(FW)/up_core.cpp -o $@

test_firmware: test_firmware.cpp shim/*.cpp shim/*.h $(FW)/*.cpp $(FW)/*.h
	g++ $(CXXFLAGS) -Ishim -I$(FW) -I$(ARDUINOJSON) test_firmware.cpp shim/stubs.cpp \
	  $(FW)/app.cpp $(FW)/report.cpp $(FW)/storage.cpp $(FW)/settings.cpp $(FW)/up_core.cpp -o $@

clean:
	rm -rf test_core test_firmware out
