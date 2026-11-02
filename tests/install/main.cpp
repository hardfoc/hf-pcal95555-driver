// Builds against the installed headers only: pcal95555.hpp, the .ipp next to it and the
// generated pcal95555_version.h.
#include "pcal95555.hpp"

#include <cstddef>
#include <cstdint>

class NullBus : public pcal95555::I2cInterface<NullBus> {
public:
  bool Write(uint8_t, uint8_t, const uint8_t*, size_t) noexcept { return false; }
  bool Read(uint8_t, uint8_t, uint8_t*, size_t) noexcept { return false; }
  bool EnsureInitialized() noexcept { return true; }
};

int main() {
  NullBus bus;
  pcal95555::PCAL95555<NullBus> gpio(&bus, uint8_t{0x20});
  // No device answers: initialisation must fail cleanly.
  return gpio.EnsureInitialized() ? 1 : 0;
}
