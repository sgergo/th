# Prefix for the arm-eabi-none toolchain.
COMPILER = arm-none-eabi

# MCU properties
PART=TM4C123GH6PM
CPU=-mcpu=cortex-m4
FPU=-mfpu=fpv4-sp-d16 -mfloat-abi=softfp

# Project path
PROJ_PATH=${shell pwd}

# Tivaware paths
TIVAWARE_PATH=~/ti/tivaware

# Other lib paths
LIBD_PATH=${TIVAWARE_PATH}/driverlib/gcc/libdriver.a
LIBUSB_PATH=${TIVAWARE_PATH}/usblib/gcc/libusb.a

# VPATH
VPATH=adc ble board diagnostic usb 

#Build directory name
OUTDIR = build

# Program name definition for ARM GNU C compiler.
CC      = ${COMPILER}-gcc
# Program name definition for ARM GNU Linker.
LD      = ${COMPILER}-gcc
# Program name definition for ARM GNU Object copy.
CP      = ${COMPILER}-objcopy
# Program name definition for ARM GNU Object dump.
OD      = ${COMPILER}-objdump
# Program name definition for ARM GNU Debugger.
DB		= ${COMPILER}-gdb
# Size information
SIZE 	= ${COMPILER}-size

# Core flags
COREFLAGS =-mthumb ${CPU} ${FPU} 
COREFLAGS += -DTARGET_IS_TM4C123_RB1
COREFLAGS += -Dgcc

# Additional flags to the compiler
CFLAGS= -g ${COREFLAGS}
CFLAGS+=-O0 -ffunction-sections -fdata-sections -MD -std=c99 -Wall -pedantic -c -g

# Library paths passed as flags.
CFLAGS+= -I ${TIVAWARE_PATH} -DPART_$(PART)
CFLAGS+= -I ${PROJ_PATH}

# Flags for linker
LFLAGS = $(COREFLAGS) -T $(LINKER_FILE) -Wl,--entry=ResetISR,--gc-sections

# Flags for objcopy
CPFLAGS = -Obinary

# Flags for objectdump
ODFLAGS = -S

# Obtain the path to libgcc, libc.a and libm.a for linking from gcc frontend.
LIB_GCC_PATH=${shell ${CC} ${CFLAGS} -print-libgcc-file-name}
LIBC_PATH=${shell ${CC} ${CFLAGS} -print-file-name=libc.a}
LIBM_PATH=${shell ${CC} ${CFLAGS} -print-file-name=libm.a}

# Uploader tool path.
PROGRAMMER=lm4flash

# On chip debugger 
OC_DEBUGGER=openocd
# Config file absolute path
CONFIG_PATH=/usr/share/openocd/scripts/boards/ek-tm4c123gxl.cfg    


#==============================================================================
#                         Project properties
#==============================================================================

# Project name (name of main file)
PROJECT_NAME = main
# Startup file name
STARTUP_FILE = startup_gcc
# Linker file name
LINKER_FILE = $(PART).ld

SRC = $(shell find . -name '[!.]*.c' -not -path "./contrib/*")
OBJS = $(addprefix $(OUTDIR)/,$(notdir $(SRC:.c=.o)))
RM      = rm -f
MKDIR	= mkdir -p

# Output text color codes
BLUE = '\033[0;34m'
GREEN = '\033[0;32m'
YELLOW = '\033[0;33m'
NC='\033[0m'

#==============================================================================
#                      Rules to make the target
#==============================================================================

#make all rule
all:$(OBJS) $(OUTDIR)/${PROJECT_NAME}.axf $(OUTDIR)/${PROJECT_NAME}

$(OUTDIR)/%.o: %.c | $(OUTDIR)
	@echo
	@echo -e $(YELLOW)Compiling $<...$(NC)
	$(CC) -c $(CFLAGS) ${<} -o ${@}

$(OUTDIR)/${PROJECT_NAME}.axf: $(OBJS)
	@echo
#	@echo Making driverlib
#	$(MAKE) -C ${TIVAWARE_PATH}/driverlib/
#	@echo
	@echo -e $(BLUE)Linking...$(NC)
	$(LD) $(LFLAGS) -o $(OUTDIR)/${PROJECT_NAME}.axf $(OBJS) $(LIBM_PATH) $(LIBC_PATH) $(LIB_GCC_PATH) $(LIBUSB_PATH) $(LIBD_PATH)

$(OUTDIR)/${PROJECT_NAME}: $(OUTDIR)/${PROJECT_NAME}.axf
	@echo
	@echo -e $(GREEN)Copying...$(NC)
	$(CP) $(CPFLAGS) $(OUTDIR)/${PROJECT_NAME}.axf $(OUTDIR)/${PROJECT_NAME}.bin
	@echo
	@echo -e $(GREEN)Creating list file...$(NC)
	$(OD) $(ODFLAGS) $(OUTDIR)/${PROJECT_NAME}.axf > $(OUTDIR)/${PROJECT_NAME}.lst
	@echo -e $(GREEN)'Printing size...(total of 256kB flash / 32kB SRAM)'$(NC)
	$(SIZE) $(OUTDIR)/${PROJECT_NAME}.axf
	
# create the output directory
$(OUTDIR):
	$(MKDIR) $(OUTDIR)

# make clean rule
clean:
	@echo Cleaning $<...
	-$(RM) $(OUTDIR)/*
# Rule to load the project to the board (sudo may be needed if rule is note made).
install:
	@echo Loading the ${PROJECT_NAME}.bin
	${PROGRAMMER} $(OUTDIR)/${PROJECT_NAME}.bin

#   GDB:
#	target extended-remote :3333
#	monitor reset halt
#	load
#	monitor reset init

#debug:
#	@echo -e 'gdb commands:\n   target extended-remote :3333\n   monitor reset halt\n   load\n   monitor reset init\n'
#	${TERMINAL} -x ${DB} ${PROJECT_NAME}.axf
#	${OC_DEBUGGER} -f ${CONFIG_PATH}
	
#test:
#	@echo -e 'gdb commands:\n   target extended-remote :3333\n   monitor reset halt\n   load\n   monitor reset init\n'

