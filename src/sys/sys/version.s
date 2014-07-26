/* also update the version in sys/punix.h */
.section _st1,"r"
.global uname_sysname, uname_nodename, uname_release, uname_version, uname_machine
uname_sysname:
	.incbin "uname.sysname"
	.byte 0
uname_nodename:
	.incbin "uname.nodename"
	.byte 0
.align	4
uname_release:
	.incbin "uname.release"
	.byte 0
	.incbin "stupiddate"
	.byte 0
uname_version:
	.incbin "uname.version"
	.byte 0
uname_machine:
	.incbin "uname.machine"
	.byte 0

.global build_date
build_date:
	.long REALTIME
