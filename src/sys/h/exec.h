struct exec {
	int		a_magic;	/* magic number */
	unsigned int	a_text;		/* size of text segment */
	unsigned int	a_data;		/* size of initialized data */
	unsigned int	a_bss;		/* size of uninitialized data */
	unsigned int	a_syms;		/* size of symbol table */
	unsigned int	a_entry;	/* entry point */
	unsigned int	a_unused;	/* not used */
	unsigned int	a_flag;		/* relocation info stripped */
};
