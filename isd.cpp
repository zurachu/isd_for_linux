#include "piecedev.h"
#include "piecefat.h"
#include <cstdio>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

static void usage( std::FILE *out )
{
	std::fputs("isd -l -- list file\n",out);
	std::fputs("isd -r filename -- download file\n",out);
	std::fputs("isd -c -- device check only\n",out);
	std::fputs("isd -s filename -- upload file\n",out);
	std::fputs("isd -f -- show file system status\n",out);
}

static int download( n::piece::Device &d, n::piece::Fs &fs, char *fname )
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

static int upload( n::piece::Device &d, n::piece::Fs &fs, char *fname )
{
	struct stat buf;
	if ( stat(fname,&buf) < 0 ) {
		std::perror("stat");
		return 1;
	}

	size_t size = buf.st_size;
	FILE *fp = fopen( fname, "rb" );

	if ( fp == NULL ){
		std::perror("fopen");
		return 1;
	}

	fs.createFile( fname, size );
	n::piece::Fs::File file( fs, fname );

	file.upload( fp );

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
			int c = getopt( argc, argv, "lr:d:c?hs:fF" );
			
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
