.PHONY: all clear clean purge check

all:
	make -C UnitTests all
	make -C TestBlinkLedBasic all
	
clean:
	make -C UnitTests clean
	make -C TestBlinkLedBasic clean
	
clear:
	make -C UnitTests clear
	make -C TestBlinkLedBasic clear
	
purge:
	make -C UnitTests purge
	make -C TestBlinkLedBasic purge
	
check:
	UnitTests/unit_tests
	TestBlinkLedBasic/test_blink_led_basic
