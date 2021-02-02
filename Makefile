.PHONY: all clear clean purge check

all:
	make -C UnitTests all
	make -C TestBlinkLedBasic all
	make -C TestFunshieldButtonsAndLeds all
	make -C TestFunshieldSegDisplay all
	
clean:
	make -C UnitTests clean
	make -C TestBlinkLedBasic clean
	make -C TestFunshieldButtonsAndLeds clean
	make -C TestFunshieldSegDisplay clean
	
clear:
	make -C UnitTests clear
	make -C TestBlinkLedBasic clear
	make -C TestFunshieldButtonsAndLeds clear
	make -C TestFunshieldSegDisplay clear
	
purge:
	make -C UnitTests purge
	make -C TestBlinkLedBasic purge
	make -C TestFunshieldButtonsAndLeds purge
	make -C TestFunshieldSegDisplay purge
	
check:
	UnitTests/unit_tests
	TestBlinkLedBasic/test_blink_led_basic
	TestFunshieldButtonsAndLeds/test_funshield_buttons_leds
	TestFunshieldSegDisplay/test_funshield_seg_display
