#! /bin/sh

PROGRAMNAME="`basename "$0"`"

usage()
{
	echo "Usage:  $PROGRAMNAME address filebase [...]" >&2
	exit 1
}

if [ $# -lt 1 ]; then usage; fi

address="$1"
shift

# proc.h
NPROC=32
SIZEOFSTRUCTPROC=400	# bigger than it actually is

for f in "$@"; do
	deffile="$f.def"; cfile="$f.h"; sfile="$f.inc"
	>"$cfile"; >"$sfile"
	while read line; do
		#set $line	# this expands '*'s to a list of files (bad)
		#symname="$1"; shift
		#symsize="$1"; shift
		#symtype="$@"
		
		# comments (comments begin with '#')
		line="`echo "$line" |
		  sed 's/^\([^#]*\)\([	 ]*#.*\)$/\1/'`"
		
		# blank lines (after removing comments)
		if echo "$line" | grep -q '^[ \t]*$'; then
			continue;
		fi
		
		symname="G_`echo "$line"|cut -f 1`"
		symsize="`echo "$line"|cut -f 2`"
		symindi="`echo "$line"|cut -f 3`" # levels of indirection (0 for arrays/struct/unions, 1 for regular variables)
		symtype="`echo "$line"|cut -f 4-`"
		address="`printf "0x%04X" "$address"`"
		echo "$symname	= $address" >>"$sfile"
			(
		echo -n "#define $symname	("
		for ((i = 0; i < symindi; ++i)); do echo -n "*"; done
		echo "($symtype *)$address)"
			) >>"$cfile"
		((address += symsize))
	done <"$deffile"
done
