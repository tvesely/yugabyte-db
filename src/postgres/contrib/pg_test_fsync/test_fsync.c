/*
 * src/tools/fsync/test_fsync.c
 *
 *
 *	test_fsync.c
 *		tests all supported fsync() methods
 */

#include "postgres.h"

#include "getopt_long.h"
#include "access/xlog_internal.h"
#include "access/xlog.h"
#include "access/xlogdefs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>


/* 
 * put the temp files in the local directory
 * unless the user specifies otherwise 
 */
#define FSYNC_FILENAME	"./test_fsync.out"

#define WRITE_SIZE	(8 * 1024)	/* 8k */

#define LABEL_FORMAT		"        %-32s"
#define NA_FORMAT			LABEL_FORMAT "%18s"
#define OPS_FORMAT			"%9.3f ops/sec"

int			ops_per_test = 2000;
char	    full_buf[XLOG_SEG_SIZE], *buf, *filename = FSYNC_FILENAME;
struct timeval start_t, stop_t;


void		handle_args(int argc, char *argv[]);
void		prepare_buf(void);
void		test_open(void);
void		test_non_sync(void);
void		test_sync(int writes_per_op);
void		test_open_syncs(void);
void		test_open_sync(const char *msg, int writes_size);
void		test_file_descriptor_sync(void);
void		print_elapse(struct timeval start_t, struct timeval stop_t);
void		die(char *str);


int
main(int argc, char *argv[])
{
	handle_args(argc, argv);
	
	prepare_buf();

	test_open();
	
	/* Test using 1 8k write */
	test_sync(1);

	/* Test using 2 8k writes */
	test_sync(2);
	
	test_open_syncs();

	test_file_descriptor_sync();
	
	test_non_sync();
	
	unlink(filename);

	return 0;
}

void
handle_args(int argc, char *argv[])
{
	static struct option long_options[] = {
		{"filename", required_argument, NULL, 'f'},
		{"ops-per-test", required_argument, NULL, 'o'},
		{NULL, 0, NULL, 0}
	};
	int			option;			/* Command line option */
	int			optindex = 0;	/* used by getopt_long */

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 ||
			strcmp(argv[1], "-?") == 0)
		{
			fprintf(stderr, "test_fsync [-f filename] [ops-per-test]\n");
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			fprintf(stderr,"test_fsync " PG_VERSION "\n");
			exit(0);
		}
	}

	while ((option = getopt_long(argc, argv, "f:o:",
			long_options, &optindex)) != -1)
	{
		switch (option)
		{
			case 'f':
				filename = strdup(optarg);
				break;

			case 'o':
				ops_per_test = atoi(optarg);
				break;
				
			default:
				fprintf(stderr,
					   "Try \"%s --help\" for more information.\n",
					   "test_fsync");
				exit(1);
				break;
		}
	}

	printf("%d operations per test\n", ops_per_test);
}

void
prepare_buf(void)
{
	int			ops;

	/* write random data into buffer */
	for (ops = 0; ops < XLOG_SEG_SIZE; ops++)
		full_buf[ops] = random();

	buf = (char *) TYPEALIGN(ALIGNOF_XLOG_BUFFER, full_buf);
}

void
test_open(void)
{
	int			tmpfile;

	/* 
	 * test if we can open the target file 
	 */
	if ((tmpfile = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1)
		die("Cannot open output file.");
	if (write(tmpfile, full_buf, XLOG_SEG_SIZE) != XLOG_SEG_SIZE)
		die("write failed");

	/* fsync now so that dirty buffers don't skew later tests */
	if (fsync(tmpfile) != 0)
		die("fsync failed");

	close(tmpfile);
}

void
test_sync(int writes_per_op)
{
	int			tmpfile, ops, writes;
	bool		fs_warning = false;
	
	if (writes_per_op == 1)
		printf("\nCompare file sync methods using one 8k write:\n");
	else
		printf("\nCompare file sync methods using two 8k writes:\n");
	printf("(in wal_sync_method preference order, except fdatasync\n");
	printf("is Linux's default)\n");

	/*
	 * Test open_datasync if available
	 */
#ifdef OPEN_DATASYNC_FLAG
	printf(LABEL_FORMAT, "open_datasync"
#if PG_O_DIRECT != 0
		" (non-direct I/O)*"
#endif
		);
	fflush(stdout);

	if ((tmpfile = open(filename, O_RDWR | O_DSYNC, 0)) == -1)
		die("Cannot open output file.");
	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		for (writes = 0; writes < writes_per_op; writes++)
			if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
				die("write failed");
		if (lseek(tmpfile, 0, SEEK_SET) == -1)
			die("seek failed");
	}
	gettimeofday(&stop_t, NULL);
	close(tmpfile);
	print_elapse(start_t, stop_t);

	/*
	 * If O_DIRECT is enabled, test that with open_datasync
	 */
#if PG_O_DIRECT != 0
	if ((tmpfile = open(filename, O_RDWR | O_DSYNC | PG_O_DIRECT, 0)) == -1)
	{
		printf(NA_FORMAT, "o_direct", "n/a**\n");
		fs_warning = true;
	}
	else
	{
		printf(LABEL_FORMAT, "open_datasync (direct I/O)");
		fflush(stdout);

		gettimeofday(&start_t, NULL);
		for (ops = 0; ops < ops_per_test; ops++)
		{
			for (writes = 0; writes < writes_per_op; writes++)
				if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
					die("write failed");
			if (lseek(tmpfile, 0, SEEK_SET) == -1)
				die("seek failed");
		}
		gettimeofday(&stop_t, NULL);
		close(tmpfile);
		print_elapse(start_t, stop_t);
	}
#endif

#else
	printf(NA_FORMAT, "open_datasync", "n/a\n");
#endif

/*
 * Test fdatasync if available
 */
#ifdef HAVE_FDATASYNC
	printf(LABEL_FORMAT, "fdatasync");
	fflush(stdout);

	if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
		die("Cannot open output file.");
	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		for (writes = 0; writes < writes_per_op; writes++)
			if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
				die("write failed");
		fdatasync(tmpfile);
		if (lseek(tmpfile, 0, SEEK_SET) == -1)
			die("seek failed");
	}
	gettimeofday(&stop_t, NULL);
	close(tmpfile);
	print_elapse(start_t, stop_t);
#else
	printf(NA_FORMAT, "fdatasync", "n/a\n");
#endif

/*
 * Test fsync
 */
	printf(LABEL_FORMAT, "fsync");
	fflush(stdout);

	if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
		die("Cannot open output file.");
	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		for (writes = 0; writes < writes_per_op; writes++)
			if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
				die("write failed");
		if (fsync(tmpfile) != 0)
			die("fsync failed");
		if (lseek(tmpfile, 0, SEEK_SET) == -1)
			die("seek failed");
	}
	gettimeofday(&stop_t, NULL);
	close(tmpfile);
	print_elapse(start_t, stop_t);
	
/*
 * If fsync_writethrough is available, test as well
 */	
#ifdef HAVE_FSYNC_WRITETHROUGH
	printf(LABEL_FORMAT, "fsync_writethrough");
	fflush(stdout);

	if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
		die("Cannot open output file.");
	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		for (writes = 0; writes < writes_per_op; writes++)
			if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
				die("write failed");
		if (fcntl(tmpfile, F_FULLFSYNC ) != 0)
			die("fsync failed");
		if (lseek(tmpfile, 0, SEEK_SET) == -1)
			die("seek failed");
	}
	gettimeofday(&stop_t, NULL);
	close(tmpfile);
	print_elapse(start_t, stop_t);
#else
	printf(NA_FORMAT, "fsync_writethrough", "n/a\n");
#endif

/*
 * Test open_sync if available
 */
#ifdef OPEN_SYNC_FLAG
	printf(LABEL_FORMAT, "open_sync"
#if PG_O_DIRECT != 0
		" (non-direct I/O)*"
#endif
		);
	fflush(stdout);

	if ((tmpfile = open(filename, O_RDWR | OPEN_SYNC_FLAG, 0)) == -1)
		die("Cannot open output file.");
	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		for (writes = 0; writes < writes_per_op; writes++)
			if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
				die("write failed");
		if (lseek(tmpfile, 0, SEEK_SET) == -1)
			die("seek failed");
	}
	gettimeofday(&stop_t, NULL);
	close(tmpfile);
	print_elapse(start_t, stop_t);

	/*
	 * If O_DIRECT is enabled, test that with open_sync
	 */
#if PG_O_DIRECT != 0
	if ((tmpfile = open(filename, O_RDWR | OPEN_SYNC_FLAG | PG_O_DIRECT, 0)) == -1)
	{
		printf(NA_FORMAT, "o_direct", "n/a**\n");
		fs_warning = true;
	}
	else
	{
		printf(LABEL_FORMAT, "open_sync (direct I/O)");
		fflush(stdout);

		gettimeofday(&start_t, NULL);
		for (ops = 0; ops < ops_per_test; ops++)
		{
			for (writes = 0; writes < writes_per_op; writes++)
				if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
					die("write failed");
			if (lseek(tmpfile, 0, SEEK_SET) == -1)
				die("seek failed");
		}
		gettimeofday(&stop_t, NULL);
		close(tmpfile);
		print_elapse(start_t, stop_t);
	}
#endif

#else
	printf(NA_FORMAT, "open_sync", "n/a\n");
#endif

#if defined(OPEN_DATASYNC_FLAG) || defined(OPEN_SYNC_FLAG)
	if (PG_O_DIRECT != 0)
		printf("* This non-direct I/O mode is not used by Postgres.\n");
#endif

	if (fs_warning)
	{
		printf("** This file system and its mount options do not support direct\n");
		printf("I/O, e.g. ext4 in journaled mode.\n");
	}
}

void
test_open_syncs(void)
{
	printf("\nCompare open_sync with different write sizes:\n");
	printf("(This is designed to compare the cost of writing 16k\n");
	printf("in different write open_sync sizes.)\n");

	test_open_sync(" 1 16k open_sync write", 16);
	test_open_sync(" 2  8k open_sync writes", 8);
	test_open_sync(" 4  4k open_sync writes", 4);
	test_open_sync(" 8  2k open_sync writes", 2);
	test_open_sync("16  1k open_sync writes", 1);
}


void
test_open_sync(const char *msg, int writes_size)
{
	int		tmpfile, ops, writes;

/*
 * Test open_sync with different size files
 */
#ifdef OPEN_SYNC_FLAG
	if ((tmpfile = open(filename, O_RDWR | OPEN_SYNC_FLAG | PG_O_DIRECT, 0)) == -1)
		printf(NA_FORMAT, "o_direct", "n/a**\n");
	else
	{
		printf(LABEL_FORMAT, msg);
		fflush(stdout);

		gettimeofday(&start_t, NULL);
		for (ops = 0; ops < ops_per_test; ops++)
		{
			for (writes = 0; writes < 16 / writes_size; writes++)
				if (write(tmpfile, buf, writes_size) != writes_size)
					die("write failed");
			if (lseek(tmpfile, 0, SEEK_SET) == -1)
				die("seek failed");
		}
		gettimeofday(&stop_t, NULL);
		close(tmpfile);
		print_elapse(start_t, stop_t);
	}
	
#else
	printf(NA_FORMAT, "open_sync", "n/a\n");
#endif
}

void
test_file_descriptor_sync(void)
{
	int			tmpfile, ops;

	/*
	 * Test whether fsync can sync data written on a different
	 * descriptor for the same file.  This checks the efficiency
	 * of multi-process fsyncs against the same file.
	 * Possibly this should be done with writethrough on platforms
	 * which support it.
	 */
	printf("\nTest if fsync on non-write file descriptor is honored:\n");
	printf("(If the times are similar, fsync() can sync data written\n");
	printf("on a different descriptor.)\n");

	/* 
	 * first write, fsync and close, which is the 
	 * normal behavior without multiple descriptors
	 */
	printf(LABEL_FORMAT, "write, fsync, close");
	fflush(stdout);

	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("Cannot open output file.");
		if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
			die("write failed");
		if (fsync(tmpfile) != 0)
			die("fsync failed");
		close(tmpfile);
		/*
		 * open and close the file again to be consistent
		 * with the following test
		 */
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("Cannot open output file.");
		close(tmpfile);
	}
	gettimeofday(&stop_t, NULL);
	print_elapse(start_t, stop_t);

	/*
	 * Now open, write, close, open again and fsync
	 * This simulates processes fsyncing each other's
	 * writes.
	 */
 	printf(LABEL_FORMAT, "write, close, fsync");
 	fflush(stdout);

	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("Cannot open output file.");
		if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
			die("write failed");
		close(tmpfile);
		/* reopen file */
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("Cannot open output file.");
		if (fsync(tmpfile) != 0)
			die("fsync failed");
		close(tmpfile);
	}
	gettimeofday(&stop_t, NULL);
	print_elapse(start_t, stop_t);

}

void
test_non_sync(void)
{
	int			tmpfile, ops;

	/*
	 * Test a simple write without fsync
	 */
	printf("\nNon-sync'ed 8k writes:\n");
	printf(LABEL_FORMAT, "write");
	fflush(stdout);

	gettimeofday(&start_t, NULL);
	for (ops = 0; ops < ops_per_test; ops++)
	{
		if ((tmpfile = open(filename, O_RDWR, 0)) == -1)
			die("Cannot open output file.");
		if (write(tmpfile, buf, WRITE_SIZE) != WRITE_SIZE)
			die("write failed");
		close(tmpfile);
	}
	gettimeofday(&stop_t, NULL);
	print_elapse(start_t, stop_t);
}

/* 
 * print out the writes per second for tests
 */
void
print_elapse(struct timeval start_t, struct timeval stop_t)
{
	double		total_time = (stop_t.tv_sec - start_t.tv_sec) +
	(stop_t.tv_usec - start_t.tv_usec) * 0.000001;
	double		per_second = ops_per_test / total_time;

	printf(OPS_FORMAT "\n", per_second);
}

void
die(char *str)
{
	fprintf(stderr, "%s\n", str);
	exit(1);
}
