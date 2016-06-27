#################################################################
# rule
# path must be a directory, exist path/project.mk,path/filelist.mk
#
# pixxx=script/default
# pixxx_arg=[all,mlib,elf...] one of us, [all, mlib,elf...] is top path Makefile operation
	pi1=script/default
		pi1_arg=elf
	pi2=script/libepollserver
		pi2_arg=mlib
	pi3=script/libminishell
		pi3_arg=mlib
# default project item
export DP=pi1
