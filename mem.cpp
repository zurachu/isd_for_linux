#include "piecedev.h"

#include <stdint.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	n::piece::Device dev;
	int mode;

	uint32_t begin, len;

	if ( argv[1][0] == 'w' )
		mode = 0;
	else
		mode = 1;

	begin = strtol( argv[2], NULL, 0 );
	len = strtol( argv[3], NULL, 0 );

	fprintf( stderr, "%x %x\n", begin, len );
	
	char buf[len];

	if ( mode == 1 ) {
		dev.readMem( begin, buf, len );
		fwrite( buf, 1, len, stdout );
	}

	return 0;
}
