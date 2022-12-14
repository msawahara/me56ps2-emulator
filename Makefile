TARGET = me56ps2
OBJS = me56ps2.o usb_raw_gadget.o usb_raw_control_event.o tcp_sock.o
CXXFLAGS = -Wall -Wextra
LDFLAGS = -pthread

.SUFFIXES: .cpp .o

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(LDFLAGS) $^

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<

.PHONY: rpi4
rpi4: $(TARGET)

.PHONY: rpi-zero
rpi-zero:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DHW_RPI_ZERO" $(TARGET)

.PHONY: rpi-zero2
rpi-zero2:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DHW_RPI_ZERO2" $(TARGET)

.PHONY: nanopi-neo2
nanopi-neo2:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DHW_NANOPI_NEO2" $(TARGET)

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)
