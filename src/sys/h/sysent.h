extern struct sysent {
	int	sy_narg; /* number of _words_ of args */
	void	(*sy_call)();
	int	sy_flags;
} sysent[];

extern const int nsysent;
extern const char *sysname[];
