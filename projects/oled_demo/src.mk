# See No-OS/tool/scripts/src_model.mk for variable description
SRCS += $(PROJECT)/src/app/main.c                                       \
        $(DRIVERS)/display/ssd_1306/ssd_1306.c                          \
        $(DRIVERS)/display/oled/oled.c                                  \
	$(DRIVERS)/gpio/gpio.c                                          \
        $(DRIVERS)/spi/spi.c                                            \
        $(DRIVERS)/platform/xilinx/xilinx_spi.c                         \
        $(DRIVERS)/platform/xilinx/xilinx_gpio.c                        

INCS += $(INCLUDE)/gpio.h						\
	$(INCLUDE)/spi.h                                                \
        $(DRIVERS)/display/ssd_1306/ssd_1306.h                          \
        $(DRIVERS)/display/oled/oled.h                                  \
        $(PROJECT)/src/app/parameters.h                                 \
        $(INCLUDE)/error.h                                              \
        $(INCLUDE)/delay.h                                              \
        $(DRIVERS)/platform/xilinx/gpio_extra.h                         \
	$(DRIVERS)/platform/xilinx/spi_extra.h