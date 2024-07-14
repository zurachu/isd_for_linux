#ifndef __PIECEFAT_H_
#define __PIECEFAT_H_

#include <stdint.h>
#include <cstdio>

namespace n {
namespace piece {

class Device;

class Fs
{
	uint32_t mblk_adr_;

	static const int MAXDIR=96;
	static const int MAXFAT=496;
	static const uint16_t FAT_FREE=0xffff;
	static const uint16_t FAT_END=0xeeee;
	static const size_t FNAME_LEN=23;

	struct Directory {
		char name[FNAME_LEN+1];
		unsigned char attr;
		unsigned char resv;
		uint16_t chain;
		uint32_t size;
	};

	struct PffsMaster {
		uint32_t ptr;
		uint32_t resv;
		char signature[24];

		Directory directory[MAXDIR];

		uint16_t fat_chain[MAXFAT];
	} master_block_ __attribute__ ((packed)) ;

	Device &dev_;

	void load();
	void update();		// 変更をむこーに反映させる

	Directory *searchFile( const char *fname );
	Directory *freeDir();

	static const int SECTOR_SIZE=4096;

	bool dirty_;

	void removeFile( Directory *d );
	inline int need_block( size_t filesize );

	void makeChain( uint16_t *pchain, int blkcnt, int next );
public:

	class File {
		Fs &fs_;
		uint16_t chain_top_;
		uint16_t chain_now_;

		int count_;

		int size_;

		uint32_t pffs_top_;

		void nextSector();
	public:
		File( Fs &fat, const char *fname );
		void download( std::FILE *out );
		void upload( std::FILE *in );

		void readSector( char buf[], size_t len );
		void writeSector( const char buf[], size_t len );
	};

	Fs( Device &dev )
		: dev_(dev), dirty_(false)
		{
			load();
		}
	~Fs();

	void removeFile( const char *fname );
	void dumpDir();

	void createFile( const char *fname, size_t len );

	size_t getFreeBlockCount( );

	void format( );
};

}
}


#endif
