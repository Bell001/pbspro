/*
 * Copyright (C) 1994-2018 Altair Engineering, Inc.
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
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Commercial License Information:
 *
 * For a copy of the commercial license terms and conditions,
 * go to: (http://www.pbspro.com/UserArea/agreement.html)
 * or contact the Altair Legal Department.
 *
 * Altair’s dual-license business model allows companies, individuals, and
 * organizations to create proprietary derivative works of PBS Pro and
 * distribute them - whether embedded or bundled with other software -
 * under a commercial license agreement.
 *
 * Use of Altair’s trademarks, including but not limited to "PBS™",
 * "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's
 * trademark licensing policies.
 *
 */
/**
 * @file    pbs_migrate_users.c
 *
 * @brief
 *		This program exists to give a way to update
 *		a user's password to the server.
 *
 * @par	usage:	pbs_migrate_users  old_server_name new_server_name
 */

#include	<pbs_config.h>   /* the master config generated by configure */
#include	<pbs_version.h>
#include	<assert.h>

#include	"cmds.h"
#include	"portability.h"
#include	"pbs_ifl.h"
#include	"libpbs.h"

#include "credential.h"
#include "ticket.h"

/**
 * @brief
 *	The main function in C - entry point
 *
 * @param[in]  argc - argument count
 * @param[in]  argv - pointer to argument array
 *
 * @return  int
 * @retval  0 - success
 * @retval  !0 - error
 */
int
main(argc, argv)
int		argc;
char	*argv[];
{
	char	*old_server;
	char	*new_server;
	int	con;
	int     ret, rc = 0;

	/*test for real deal or just version and exit*/

	PRINT_VERSION_AND_EXIT(argc, argv);

#ifdef WIN32
	if (winsock_init()) {
		return 1;
	}
#endif

	if (argc != 3) {
		fprintf(stderr,
			"usage:\t pbs_migrate_users old_server_name new_server_name...\n");
		fprintf(stderr, "      \t pbs_migrate_users --version\n");
		exit(-7);
	}


	old_server = argv[1];
	new_server = argv[2];

	if ((old_server == NULL) || (new_server == NULL)) {
		fprintf(stderr,
			"usage:\t pbs_migrate_users old_server_name new_server_name...\n");
		fprintf(stderr, "      \t pbs_migrate_users --version\n");
		exit(-5);
	}

	if (CS_client_init() != CS_SUCCESS) {
		fprintf(stderr, "pbs_migrate_users: unable to initialize security library.\n");
		exit(-6);
	}

	con= cnt2server(old_server);
	if (con <= 0) {
		fprintf(stderr, "pbs_migrate_users: cannot connect to server %s, error=%d\n",
			old_server, pbs_errno);
		CS_close_app();
		exit(-6);
	}


	ret = PBSD_user_migrate(con, new_server);

	if (ret == 0) {

		printf("Users on %s migrated to %s\n", old_server, new_server);
	} else {
		switch (ret) {
			case PBSE_SSIGNON_UNSET_REJECT:
				printf("single_signon_password_enable not set in either %s or %s\n", old_server, new_server);
				rc = -3;
				break;
			case PBSE_PERM:
				printf("%s not authorized to migrate users\n",
					getlogin());
				rc = -4;
				break;
			case PBSE_SSIGNON_NOCONNECT_DEST:
				printf("communication failure between %s and %s\n",
					old_server, new_server);
				rc =-2;
				break;
			case PBSE_SYSTEM:
			default:
				printf("getting/writing out users' passwords failed!\n");
				rc = -1;
		}
	}

	(void)pbs_disconnect(con);

	return (rc);
}
