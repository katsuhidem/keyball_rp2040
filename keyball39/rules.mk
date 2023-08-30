SERIAL_DRIVER = vendor

POINTING_DEVICE_ENABLE = yes
POINTING_DEVICE_DRIVER = pmw3360
QUANTUM_LIB_SRC += spi_master.c # Optical sensor use SPI to communicate

# Enabled only one of RGBLIGHT and RGB_MATRIX if necessary.
RGBLIGHT_ENABLE = no        # Enable RGBLIGHT
RGB_MATRIX_ENABLE = no      # Enable RGB_MATRIX (not work yet)

# Do not enable VIA_ENABLE. This firmware doesn't support EEPROM emulation
VIA_ENABLE = no

# Do not enable SLEEP_LED_ENABLE. it uses the same timer as BACKLIGHT_ENABLE
SLEEP_LED_ENABLE = no       # Breathing sleep LED during USB suspend

# To support OLED
OLED_ENABLE = yes                # Please Enable this in each keymaps.
SRC += lib/oledkit/oledkit.c    # OLED utility for Keyball series.

# Include common library
SRC += lib/keyball/keyball.c
SRC += lib/common/twpair_on_jis.c
SRC += lib/common/auto_mouse.c

# Disable other features to squeeze firmware size
SPACE_CADET_ENABLE = no
MAGIC_ENABLE = no