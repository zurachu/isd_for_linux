#include "piecedev.h"
#include "piecefat.h"
#include <string>
#include <algorithm>
#include <cstdio>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

static void usage( std::FILE *out )
{
	std::fputs("isd -c -- device check only\n",out);
	std::fputs("isd -d filename -- delete file\n",out);
	std::fputs("isd -F -- format file system\n",out);
	std::fputs("isd -f -- show file system status\n",out);
	std::fputs("isd -l -- list file\n",out);
	std::fputs("isd -R filename -- run srf file\n",out);
	std::fputs("isd -r filename -- download file\n",out);
	std::fputs("isd -s filename -- upload file\n",out);
	std::fputs("isd -k all.img@ku.srf -- update kernel\n",out);
	std::fputs("isd -v -- show version\n",out);
}

static int download( n::piece::Device &d, n::piece::Fs &fs, char const *fname )
{
	n::piece::Fs::File file( fs, fname );
	std::FILE *out = std::fopen( fname, "wb" );
	if ( out == NULL ) {
		std::perror("fopen");
		return 1;
	}

	file.download( out );
	return 0;
}

static int upload( n::piece::Device &d, n::piece::Fs &fs, char const *fname )
{
	struct stat buf;
	if ( stat( fname, &buf ) < 0 ) {
		std::perror("stat");
		return 1;
	}

	std::string uploadFileName(fname);
	size_t pathDelimiterPos = uploadFileName.rfind('/');
	if (pathDelimiterPos != std::string::npos)
		uploadFileName.erase(0, pathDelimiterPos + 1);
	std::transform(uploadFileName.begin(), uploadFileName.end(), uploadFileName.begin(), std::tolower);

	size_t size = buf.st_size;
	FILE *fp = fopen( fname, "rb" );

	if ( fp == NULL ) {
		std::perror("fopen");
		return 1;
	}

	fs.createFile( uploadFileName.c_str(), size );
	n::piece::Fs::File file( fs, uploadFileName.c_str() );

	file.upload( fp );

	fclose( fp );

	return 0;
}

static int run_srf_file( n::piece::Device &d, char *fname )
{
	struct stat buf;
	if ( stat( fname, &buf ) < 0 ) {
		std::perror("stat");
		return 1;
	}

	FILE *fp = fopen( fname, "rb" );

	if ( fp == NULL ){
		std::perror("fopen");
		return 1;
	}

	d.setAppStat( n::piece::Device::APP_STOP );
	d.uploadSrf( fp );
	d.setAppStat( n::piece::Device::APP_RUN );

	fclose( fp );

	return 0;
}

static int update_kernel( n::piece::Device &d, char *arg )
{
	char *at_sign = strchr(arg, '@');
	if ( at_sign == NULL ){
		std::fprintf( stderr, "missing '@'\n" );
		return 1;
	}

	*at_sign = '\0';

	const char *ku_path = at_sign + 1;

	char buf[512*1024];

	FILE *fp = fopen( arg, "rb" );

	if ( fp == NULL ){
		std::perror("fopen");
		return 1;
	}

	struct stat stat_buf;
	if ( stat( arg, &stat_buf ) < 0 ) {
		std::perror("stat");
		fclose( fp );
		return 1;
	}

	size_t len = stat_buf.st_size;
	if ( len >= sizeof(buf) ) {
		std::fprintf( stderr, "kernel image too large\n" );
		fclose( fp );
		return 1;
	}

	if ( fread( buf, 1, len, fp ) != len ) {
		std::perror("fread");
		fclose( fp );
		return 1;
	}

	fclose( fp );

	fp = fopen( ku_path, "rb" );
	if ( fp == NULL ){
		std::perror("fopen");
		return 1;
	}

	d.setAppStat( n::piece::Device::APP_STOP );
	d.writeMem( 0x102c00, buf, len );
	d.uploadSrf( fp );
	d.setAppStat( n::piece::Device::APP_RUN );
	fclose( fp );
	return 0;
}

static int fs_status( n::piece::Device &d, n::piece::Fs &fs )
{
	size_t size = fs.getFreeBlockCount();
	printf("%5zu sectors (%zu bytes) free\n", size, size*4096);
	return 0;
}

int main( int argc, char **argv )
{
	try {

		n::piece::Device d;
		n::piece::Fs fs( d );
		while ( 1 ) {
			int c = getopt( argc, argv, "lr:d:c?hs:fFvR:k:" );

			switch ( c ) {
			case 'l':
				fs.dumpDir();
				return fs_status( d, fs );

			case '?':
			case 'h':
				usage( stdout );
				return 0;

			case 'r':
				return download( d, fs, optarg );

			case 'd':
				fs.removeFile( optarg );
				return 0;

			case 'c':
				return 0;

			case 's':
				return upload( d, fs, optarg );

			case 'f':
				return fs_status( d, fs );

			case 'F':
				fs.format();
				return 0;

			case 'v':
				d.dumpVersion();
				return 0;

			case 'R':
				return run_srf_file( d, optarg );

			case 'k':
				return update_kernel( d, optarg );

			default:
				usage( stderr );
				return 1;
			}
		}

	} catch ( const char *err ) {
		std::fprintf( stderr, "%s\n", err );
		return 1;
	}

	return 0;
}
