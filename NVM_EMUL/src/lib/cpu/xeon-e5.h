#include "dev.h"

int intel_xeon_e5_set_throttle_register(int bus_id, throttle_type_t throttle_type, uint16_t val)
{
    int offset;

    switch(throttle_type) {
        case THROTTLE_DDR_ACT:
            offset = 0x190; break;
        case THROTTLE_DDR_READ:
            offset = 0x192; break;
        case THROTTLE_DDR_WRITE:
            offset = 0x194; break;
        default:
            offset = 0x190;
    }

    set_pci(bus_id, 0x10, 0x0, offset, (uint16_t) val);
    set_pci(bus_id, 0x10, 0x1, offset, (uint16_t) val);
    set_pci(bus_id, 0x10, 0x5, offset, (uint16_t) val);

    return 0;
}

int intel_xeon_e5_get_throttle_register(int bus_id, throttle_type_t throttle_type, uint16_t* val)
{
    int offset;

    switch(throttle_type) {
        case THROTTLE_DDR_ACT:
            offset = 0x190; break;
        case THROTTLE_DDR_READ:
            offset = 0x192; break;
        case THROTTLE_DDR_WRITE:
            offset = 0x194; break;
        default:
            offset = 0x190;
    }

    get_pci(bus_id, 0x10, 0x1, offset, val);
    return 0;
}


cpu_model_t cpu_model_intel_xeon_e5 = {
    .desc = {"Intel", "SandyBridge", "Xeon", "E5-24[0-9][0-9]"},
    .set_throttle_register = intel_xeon_e5_set_throttle_register,
    .get_throttle_register = intel_xeon_e5_get_throttle_register
};

