# ----------------------------------------------------------------------------
# OLED Display - Bank 13
# ----------------------------------------------------------------------------
set_property PACKAGE_PIN U12  [get_ports oled_vdd]
set_property PACKAGE_PIN U9   [get_ports oled_reset]
set_property PACKAGE_PIN U11  [get_ports oled_vbat]
set_property PACKAGE_PIN U10  [get_ports oled_dc]
set_property PACKAGE_PIN AB12 [get_ports oled_sclk]
set_property PACKAGE_PIN AA12 [get_ports oled_sdin]

# ----------------------------------------------------------------------------
# User LEDs - Bank 33
# ----------------------------------------------------------------------------
set_property PACKAGE_PIN T22 [get_ports led0]
set_property PACKAGE_PIN T21 [get_ports led1]
set_property PACKAGE_PIN U22 [get_ports led2]

set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 13]]
set_property IOSTANDARD LVCMOS33 [get_ports -of_objects [get_iobanks 33]]