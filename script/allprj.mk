#################################################################
# you can edit it for all target
# create all project target with sequence

.PHONY:allp
allp:pi1

pi1:
	$(MAKE) DP=pi1 --no-print-directory
# $(MAKE) DP=pi4 --no-print-directory
# $(MAKE) allstrip
	

# clean all project output
.PHONY:allclean
allclean:

	$(MAKE) DP=pi1 clean --no-print-directory
# $(MAKE) DP=pi4 clean --no-print-directory
	
# remote all output file and empty directory which create by Makefile
.PHONY:alldisclean
alldistclean:
	$(MAKE) DP=pi1 disclean --no-print-directory

	
# strip all output file STRTAB section
.PHONY:allstrip
allstrip:
	$(MAKE) DP=pi1 strip --no-print-directory
# $(MAKE) DP=pi4 strip --no-print-directory
