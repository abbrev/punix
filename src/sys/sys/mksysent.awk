#! /usr/bin/awk -f
# num  name  words  flags
# Example:
# 3    read  5      0

BEGIN {
	maxnum = 0
}

/^[^#]/ && ! /^$/ {
	num=$1;
	name=$2;
	words=$3;
	flags=$4;
	if (NF < 4) {
		printf("Warning: line %d has too few fields; skipping.\n", NR) >"/dev/stderr";
		next
	} else if (NF > 4) {
		printf("Warning: line %d has too many fields; skipping.\n", NR) >"/dev/stderr";
		next
	}
	if (calls[num, "name"] != "") {
		printf("syscall %d (%s) is redefined! (Redefinition at line %d)\n", num, name, NR) > "/dev/stderr";
		err = 1
		exit 1
	}
	calls[num, "name"] = name;
	calls[num, "words"] = words;
	calls[num, "flags"] = flags;
	if (num > maxnum) { maxnum = num; }
}

END {
	if (err) exit;

	# preamble
	print("/*\n * NOTICE: This file is auto-generated.\n * DO NOT MODIFY THIS FILE!\n */\n");
	print("#include \"sysent.h\"");
	print("#include \"punix.h\"");
	print("");

	# prototypes for all syscalls
	printf("void sys_NONE();\n");
	for (i = 0; i <= maxnum; ++i) {
		if (calls[i, "name"] == "") {
			calls[i, "name"] = "NONE";
			calls[i, "words"] = "0";
			calls[i, "flags"] = "0";
		} else {
			printf("void sys_%s();\n", calls[i, "name"]);
		}
	}
	print("");

	# sysent[] array
	print("STARTUP(const struct sysent sysent[]) = {");
	for (i = 0; i <= maxnum; ++i) {
		printf("\t{ %d, sys_%s, %s },", calls[i, "words"], calls[i, "name"], calls[i, "flags"]);
		if ((i % 5) == 0) {
			printf("\t/* %d */", i);
		}
		print "";
	}
	print("};");
	print("");
	print("const int nsysent = sizeof(sysent) / sizeof(struct sysent);");
	print("");
	
	# sysname[] array
	print("STARTUP(const char *const sysname[]) = {");
	for (i = 0; i <= maxnum; ++i) {
		printf("\t\"%s\",", calls[i, "name"]);
		if ((i % 5) == 0) {
			printf("\t/* %d */", i);
		}
		print "";
	}
	print("};");
}

