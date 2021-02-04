##############################################################################
##  This file is part of the HCAN tools suite.
##
##  HCAN is free software; you can redistribute it and/or modify it under
##  the terms of the GNU General Public License as published by the Free
##  Software Foundation; either version 2 of the License, or (at your
##  option) any later version.
##
##  HCAN is distributed in the hope that it will be useful, but WITHOUT
##  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
##  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
##  for more details.
##
##  You should have received a copy of the GNU General Public License along
##  with HCAN; if not, write to the Free Software Foundation, Inc., 51
##  Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
##
##  (c) 2017 by Ingo Lages, i (dot) lages (at) gmx (dot) de
##############################################################################

include ./ARCH.inc
.PHONY: tools
CORES = $(shell grep 'cpu cores' /proc/cpuinfo | uniq | cut -d: -f2 | xargs)

alles:
	$(MAKE) clean -j$(CORES)
	$(MAKE) all
	$(MAKE) tools -j$(CORES) xy="sudo" xx="all"
	$(MAKE) install -j$(CORES)

firmwareOnly:
	$(MAKE) clean -j$(CORES)
	$(MAKE) strukturen xy="" xx="all"
	$(MAKE) firmware xy="" xx="all" parm2=MCU=atmega328p &&  $(MAKE) firmware -j$(CORES) xy="sudo" xx="clean_part" parm2=MCU=atmega328p
	$(MAKE) firmware xy="" xx="all" parm2=MCU=atmega32 &&    $(MAKE) firmware -j$(CORES) xy="sudo" xx="clean_part" parm2=MCU=atmega32
	$(MAKE) firmware xy="" xx="all" parm2=MCU=atmega644p &&  $(MAKE) firmware -j$(CORES) xy="sudo" xx="clean_part" parm2=MCU=atmega644p

alles-silent:
	$(MAKE) alles > /dev/null

bananapi:
	$(MAKE) strukturen xy="sudo" xx="clean"
	$(MAKE) cDienste -j$(CORES) xy="sudo" xx="clean"
	$(MAKE) lib -j$(CORES) xy="sudo" xx="clean"
	$(MAKE) cppDienste -j$(CORES) xy="sudo" xx="clean"
	$(MAKE) tools xy="sudo" xx="clean"

	$(MAKE) strukturen xy="" xx="all"
	$(MAKE) cDienste -j$(CORES) xy="" xx="all"
	$(MAKE) cppDienste -j$(CORES) xy="" xx="all"

	$(MAKE) strukturen xy="sudo" xx="install"
	$(MAKE) cDienste -j$(CORES) xy="sudo" xx="install"
	$(MAKE) lib -j$(CORES) xy="sudo" xx="install"
	$(MAKE) cppDienste -j$(CORES) xy="sudo" xx="install"

	$(MAKE) tools -j$(CORES) xy="sudo" xx="all"

clean:
	$(MAKE) strukturen xy="sudo" xx="clean"
	$(MAKE) cDienste -j$(CORES) xy="sudo" xx="clean"
	$(MAKE) lib -j$(CORES) xy="sudo" xx="clean"
	$(MAKE) cppDienste -j$(CORES) xy="sudo" xx="clean"
	$(MAKE) firmware -j$(CORES) xy="sudo" xx="clean" parm2=MCU=atmega328p
	$(MAKE) firmware -j$(CORES) xy="sudo" xx="clean" parm2=MCU=atmega32
	$(MAKE) firmware -j$(CORES) xy="sudo" xx="clean" parm2=MCU=atmega644p
	$(MAKE) firmwareOhneEds xy="sudo" xx="clean"
	$(MAKE) tools xy="sudo" xx="clean"
	@# folgendes wuerde den "Dropbox-Pfad" erzwingen:  cd hcanhab2_mqtt; make clean
	#
	@#sudo find -type f -name ".depend" | xargs rm -f

all:
	$(MAKE) strukturen xy="" xx="all"
	$(MAKE) cDienste -j$(CORES) xy="" xx="all"
	$(MAKE) cppDienste -j$(CORES) xy="" xx="all"
	$(MAKE) firmware xy="" xx="all" parm2=MCU=atmega328p &&  $(MAKE) firmware -j$(CORES) xy="sudo" xx="clean_part" parm2=MCU=atmega328p
	$(MAKE) firmware xy="" xx="all" parm2=MCU=atmega32 &&    $(MAKE) firmware -j$(CORES) xy="sudo" xx="clean_part" parm2=MCU=atmega32
	$(MAKE) firmware xy="" xx="all" parm2=MCU=atmega644p &&  $(MAKE) firmware -j$(CORES) xy="sudo" xx="clean_part" parm2=MCU=atmega644p
	$(MAKE) firmwareOhneEds xy="sudo" xx="clean" && $(MAKE) firmwareOhneEds xy="" xx="all"
	
install:
	$(MAKE) strukturen xy="sudo" xx="install"
	$(MAKE) cDienste -j$(CORES) xy="sudo" xx="install"
	$(MAKE) lib -j$(CORES) xy="sudo" xx="install"
	$(MAKE) cppDienste -j$(CORES) xy="sudo" xx="install"
	##############################################################################
	# Nun kann die Firmware geladen werden (Bootloader flashen, Firmware loaden) #
	##############################################################################

#staticAnalyse:
#	$(MAKE) cDienste xx="scan-build -o ./scanBuild make all -j4"
#	$(MAKE) cppDienste xx="scan-build -o ./scanBuild make allSrc -j4"
	@# avr-clang notwendig:   make firmware xx="scan-build -o ./scanBuild make all -j4"
	
#staticAnalyseClean:
#	sudo find -type f -name "scanBuild" | xargs rm -f

strukturen:
	$(xy) $(MAKE) $(xx) -C xml

cDienste: 
	$(xy) $(MAKE) $(xx) -C hcand
	$(xy) $(MAKE) $(xx) -C hcanaddressd
	$(xy) $(MAKE) $(xx) -C hcansocketd
	$(xy) $(MAKE) $(xx) -C hcan4mqttpc

lib:
	$(xy) $(MAKE) $(xx) -C libhcan++
	$(xy) $(MAKE) $(xx) -C libhcandata

cppDienste:	
	$(xy) $(MAKE) $(xx) -C telican
	$(xy) $(MAKE) $(xx) -C check_hcan
	$(xy) $(MAKE) $(xx) -C hcanswd
	$(xy) $(MAKE) $(xx) -C hcandq

firmware: 
	$(xy) $(MAKE) $(xx) -C hcanbl $(parm2)
	$(xy) $(MAKE) $(xx) -C firmwares/controllerboard $(parm2)
#	$(xy) $(MAKE) $(xx) -C firmwares/userpanel-v01 $(parm2)
	$(xy) $(MAKE) $(xx) -C firmwares/ws2812-modul $(parm2)

firmwareOhneEds:
#	$(xy) $(MAKE) $(xx) -C firmwares/usv-modul

tools:
	$(xy) $(MAKE) $(xx) -C tools
	$(xy) $(MAKE) $(xx) -C tools/hcanextid
