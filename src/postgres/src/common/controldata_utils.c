/*-------------------------------------------------------------------------
 *
 * controldata_utils.c
 *		Common code for control data file output.
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/common/controldata_utils.c
 *
 *-------------------------------------------------------------------------
 */

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "access/xlog_internal.h"
#include "catalog/pg_control.h"
#include "common/controldata_utils.h"
#include "common/file_perm.h"
#include "port/pg_crc32c.h"
#ifndef FRONTEND
#include "storage/fd.h"
#endif

/*
 * get_controlfile()
 *
 * Get controlfile values.  The result is returned as a palloc'd copy of the
 * control file data.
 *
 * crc_ok_p can be used by the caller to see whether the CRC of the control
 * file data is correct.
 */
ControlFileData *
get_controlfile(const char *DataDir, const char *progname, bool *crc_ok_p)
{
	ControlFileData *ControlFile;
	int			fd;
	char		ControlFilePath[MAXPGPATH];
	pg_crc32c	crc;
	int			r;

	AssertArg(crc_ok_p);

	ControlFile = palloc(sizeof(ControlFileData));
	snprintf(ControlFilePath, MAXPGPATH, "%s/global/pg_control", DataDir);

#ifndef FRONTEND
	if ((fd = OpenTransientFile(ControlFilePath, O_RDONLY | PG_BINARY)) == -1)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\" for reading: %m",
						ControlFilePath)));
#else
	if ((fd = open(ControlFilePath, O_RDONLY | PG_BINARY, 0)) == -1)
	{
		fprintf(stderr, _("%s: could not open file \"%s\" for reading: %s\n"),
				progname, ControlFilePath, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif

	r = read(fd, ControlFile, sizeof(ControlFileData));
	if (r != sizeof(ControlFileData))
	{
		if (r < 0)
#ifndef FRONTEND
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not read file \"%s\": %m", ControlFilePath)));
#else
		{
			fprintf(stderr, _("%s: could not read file \"%s\": %s\n"),
					progname, ControlFilePath, strerror(errno));
			exit(EXIT_FAILURE);
		}
#endif
		else
#ifndef FRONTEND
			ereport(ERROR,
					(errcode(ERRCODE_DATA_CORRUPTED),
					 errmsg("could not read file \"%s\": read %d of %zu",
							ControlFilePath, r, sizeof(ControlFileData))));
#else
		{
			fprintf(stderr, _("%s: could not read file \"%s\": read %d of %zu\n"),
					progname, ControlFilePath, r, sizeof(ControlFileData));
			exit(EXIT_FAILURE);
		}
#endif
	}

#ifndef FRONTEND
	if (CloseTransientFile(fd))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m",
						ControlFilePath)));
#else
	if (close(fd))
	{
		fprintf(stderr, _("%s: could not close file \"%s\": %s\n"),
				progname, ControlFilePath, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif

	/* Check the CRC. */
	INIT_CRC32C(crc);
	COMP_CRC32C(crc,
				(char *) ControlFile,
				offsetof(ControlFileData, crc));
	FIN_CRC32C(crc);

	*crc_ok_p = EQ_CRC32C(crc, ControlFile->crc);

	/* Make sure the control file is valid byte order. */
	if (ControlFile->pg_control_version % 65536 == 0 &&
		ControlFile->pg_control_version / 65536 != 0)
#ifndef FRONTEND
		elog(ERROR, _("byte ordering mismatch"));
#else
		printf(_("WARNING: possible byte ordering mismatch\n"
				 "The byte ordering used to store the pg_control file might not match the one\n"
				 "used by this program.  In that case the results below would be incorrect, and\n"
				 "the PostgreSQL installation would be incompatible with this data directory.\n"));
#endif

	return ControlFile;
}

/*
 * update_controlfile()
 *
 * Update controlfile values with the contents given by caller.  The
 * contents to write are included in "ControlFile".  Note that it is up
 * to the caller to fsync the updated file, and to properly lock
 * ControlFileLock when calling this routine in the backend.
 */
void
update_controlfile(const char *DataDir, const char *progname,
				   ControlFileData *ControlFile)
{
	int			fd;
	char		buffer[PG_CONTROL_FILE_SIZE];
	char		ControlFilePath[MAXPGPATH];

	/*
	 * Apply the same static assertions as in backend's WriteControlFile().
	 */
	StaticAssertStmt(sizeof(ControlFileData) <= PG_CONTROL_MAX_SAFE_SIZE,
					 "pg_control is too large for atomic disk writes");
	StaticAssertStmt(sizeof(ControlFileData) <= PG_CONTROL_FILE_SIZE,
					 "sizeof(ControlFileData) exceeds PG_CONTROL_FILE_SIZE");

	/* Recalculate CRC of control file */
	INIT_CRC32C(ControlFile->crc);
	COMP_CRC32C(ControlFile->crc,
				(char *) ControlFile,
				offsetof(ControlFileData, crc));
	FIN_CRC32C(ControlFile->crc);

	/*
	 * Write out PG_CONTROL_FILE_SIZE bytes into pg_control by zero-padding
	 * the excess over sizeof(ControlFileData), to avoid premature EOF related
	 * errors when reading it.
	 */
	memset(buffer, 0, PG_CONTROL_FILE_SIZE);
	memcpy(buffer, ControlFile, sizeof(ControlFileData));

	snprintf(ControlFilePath, sizeof(ControlFilePath), "%s/%s", DataDir, XLOG_CONTROL_FILE);

#ifndef FRONTEND
	if ((fd = OpenTransientFile(ControlFilePath, O_WRONLY | PG_BINARY)) == -1)
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": %m",
						ControlFilePath)));
#else
	if ((fd = open(ControlFilePath, O_WRONLY | PG_BINARY,
				   pg_file_create_mode)) == -1)
	{
		fprintf(stderr, _("%s: could not open file \"%s\": %s\n"),
				progname, ControlFilePath, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif

	errno = 0;
	if (write(fd, buffer, PG_CONTROL_FILE_SIZE) != PG_CONTROL_FILE_SIZE)
	{
		/* if write didn't set errno, assume problem is no disk space */
		if (errno == 0)
			errno = ENOSPC;

#ifndef FRONTEND
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not write file \"%s\": %m",
						ControlFilePath)));
#else
		fprintf(stderr, _("%s: could not write \"%s\": %s\n"),
				progname, ControlFilePath, strerror(errno));
		exit(EXIT_FAILURE);
#endif
	}

#ifndef FRONTEND
	if (CloseTransientFile(fd))
		ereport(PANIC,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m",
						ControlFilePath)));
#else
	if (close(fd) < 0)
	{
		fprintf(stderr, _("%s: could not close file \"%s\": %s\n"),
				progname, ControlFilePath, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif
}
