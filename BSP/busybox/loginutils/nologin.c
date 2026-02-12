/* vi: set sw=4 ts=4: */
/*
 * Mini nologin implementation for busybox
 *
 * Copyright (C) 2013 by Marius Spix <marius.spix@web.de>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

//usage:#define nologin_trivial_usage
//usage:       ""
//usage:#define nologin_full_usage "\n\n"
//usage:       "politely refuse a login"

#include "libbb.h"

int nologin_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int nologin_main(int argc UNUSED_PARAM, char **argv UNUSED_PARAM)
{
	printf("This account is currently not available.");
	return(EXIT_FAILURE);
}
