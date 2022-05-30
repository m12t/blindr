# testing 3 position toggle switch

from machine import Pin
import utime

up_pin = Pin(18, Pin.IN, Pin.PULL_UP)
down_pin = Pin(19, Pin.IN, Pin.PULL_UP)


while (1):
    if (up_pin.value() == 0):
        print("Up pin triggerred")
    if (down_pin.value() == 0):
        print("Down pin triggered")
    # print(up_pin.value(), down_pin.value())
    utime.sleep_ms(100)


  
# --- alternate that only prints once per trigger ---
#up_active, down_active = 0, 0
#while (1):
#    if (up_pin.value() == 0):
#        if (not up_active):
#            print("Up pin triggerred")
#            up_active = 1
#    else:
#        up_active = 0
#    if (down_pin.value() == 0):
#        if (not down_active):
#            print("Down pin triggered")
#            down_active = 1
#    else:
#        down_active = 0
#    
#    # print(up_pin.value(), down_pin.value())
#    utime.sleep_ms(100)