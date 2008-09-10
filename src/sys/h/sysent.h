extern struct sysent {
	int	sy_narg; /* number of _words_ of args */
	void	(*sy_call)();
} sysent[];

extern const int nsysent;
extern const char *sysname[];
