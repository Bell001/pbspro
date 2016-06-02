/*
 * Copyright (C) 1994-2016 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *  
 * This file is part of the PBS Professional ("PBS Pro") software.
 * 
 * Open Source License Information:
 *  
 * PBS Pro is free software. You can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free 
 * Software Foundation, either version 3 of the License, or (at your option) any 
 * later version.
 *  
 * PBS Pro is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 * Commercial License Information: 
 * 
 * The PBS Pro software is licensed under the terms of the GNU Affero General 
 * Public License agreement ("AGPL"), except where a separate commercial license 
 * agreement for PBS Pro version 14 or later has been executed in writing with Altair.
 *  
 * Altair’s dual-license business model allows companies, individuals, and 
 * organizations to create proprietary derivative works of PBS Pro and distribute 
 * them - whether embedded or bundled with other software - under a commercial 
 * license agreement.
 * 
 * Use of Altair’s trademarks, including but not limited to "PBS™", 
 * "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's 
 * trademark licensing policies.
 *
 */
#include <pbs_config.h>   /* the master config generated by configure */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "portability.h"
#include "log.h"
#ifdef WIN32
#include "win.h"
#endif


/**
 * @brief
 *	setup the daemon's environment
 *	To provide a "safe and secure" environment, the daemons replace their
 *	inherited one with one from a file.  Each line in the file is
 *	either a comment line, starting with '#' or ' ', or is a line of
 *	the forms:	variable=value
 *			variable
 *	In the second case the value is obtained from the current environment.
 *
 * @param[in]   filen - the daemons relace their inherited one with one from this
 *                     file
 *
 * @return int
 * @retval  0	The environment was setup successfully
 *		connection made.
 * @retval -1	error encountered setting up the environment
 */
#define PBS_ENVP_STR      64
#define PBS_ENV_CHUNCK  8191

int
setup_env(char *filen)
{
	static	char	id[] = "setup_env";
	char	     buf[ PBS_ENV_CHUNCK + 1 ];
	FILE	    *efile;
	char        *envbuf;
	static char *envp[ PBS_ENVP_STR + 1 ];
	static char *nulenv[ 1 ];
	int	     questionable = 0;
	int	     i, len;
	int	     nstr = 0;
	char	    *pequal;
	char	    *pval = NULL;
	extern char **environ;

	if (environ == envp) {
		for (i=0; envp[i] != NULL; i++) {
			free(envp[i]);
			envp[i] = NULL;
		}
	}
	environ = nulenv;
	if ((filen == (char *)0) || (*filen == '\0'))
		return 0;

	efile = fopen(filen, "r");
	if (efile != (FILE *)0) {

		while (fgets(buf, PBS_ENV_CHUNCK, efile)) {

			if (questionable) {
				/* previous bufr had no '\n' and more bytes remain */
				goto err;
			}

			if ((buf[0] != '#') && (buf[0] != ' ') && (buf[0] != '\n')) {

				len = strlen(buf);
				if (buf[len - 1] != '\n') {
					/* no newline, wonder if this is last line */
					questionable = 1;
				} else {
#ifdef WIN32
					/* take care of <carriage-return> char */
					if (len > 1 && !isalnum(buf[len-2]))
						buf[len-2] = '\0';
#endif
					buf[len - 1] = '\0';
				}

				if ((pequal = strchr(buf, (int)'=')) == 0) {
					if ((pval = getenv(buf)) == 0)
						continue;
					len += strlen(pval) + 1;
				}

				if ((envbuf = malloc(len + 1)) == 0)
					goto err;
				(void)strcpy(envbuf, buf);
				if (pequal == 0) {
					(void)strcat(envbuf, "=");
					(void)strcat(envbuf, pval);
				}
				envp[nstr++] = envbuf;
				if (nstr == PBS_ENVP_STR)
					goto err;
				envp[nstr] = (char *)0;
			}
		}
		fclose(efile);
		environ = envp;
	} else if (errno != ENOENT) {
		goto err;
	}
#ifdef WIN32
	update_env_avltree();
#endif
	sprintf(log_buffer, "read environment from %s", filen);
	log_event(PBSEVENT_SYSTEM, 0, LOG_NOTICE, id, log_buffer);
	return (nstr);

	err:	log_err(errno, id, "Could not set up the environment");
	fclose(efile);
	return (-1);
}