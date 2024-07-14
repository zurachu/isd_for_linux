#include "piecefat.h"
#include "piecedev.h"

#include "debug.h"

#include <cstring>
#include <cstdio>

namespace {
void fileNotFound( const char *fname )
{
	static char tmp[64];
	snprintf( tmp, sizeof(tmp), "%s: file not found", fname );

	throw tmp;
}
}
namespace n {
namespace piece {

void Fs::load( )
{
	mblk_adr_ = dev_.getMasterBlockAdr();
	dev_.readMem( mblk_adr_, (char*)&master_block_, sizeof(master_block_) );

	DEBUG_MSG("%x %x %s\n", master_block_.ptr, master_block_.resv, master_block_.signature );
}

void Fs::dumpDir( )
{
	Directory *d = master_block_.directory;
	for ( int i=0; i<MAXDIR; i++ ) {
		const char *name = d[i].name;

		if ( (*name!=0) && (*name!=-1) )
			std::printf( "+%3d:%6u %s\n", i, d[i].size, name );
	}
}

Fs::Directory *Fs::searchFile( const char *fname )
{
	Fs::Directory *d = master_block_.directory;
	for ( int i=0; i<MAXDIR; i++ ) {
		const char *name = d[i].name;

		if ( std::strcmp( name, fname ) == 0 )
			return &d[i];
	}

	return NULL;
}

Fs::Directory *Fs::freeDir( )
{
	Fs::Directory *d = master_block_.directory;
	for ( int i=0; i<MAXDIR; i++ ) {
		const char *name = d[i].name;

		if ( (name[0]==0) || (name[1]==-1) )
			return &d[i];
	}
	
	return NULL;
}

void Fs::removeFile( const char *fname )
{
	Fs::Directory *d = searchFile( fname );
	if ( d == NULL )
		fileNotFound( fname );

	removeFile( d );
}

void Fs::removeFile( Directory *d )
{
	uint16_t *fat = master_block_.fat_chain;
	int adr = d->chain;

	std::memset( d, 0xff, sizeof(*d) );

	while ( adr<MAXFAT ) {
		int tmp = fat[adr];
		fat[adr] = FAT_FREE;
		adr = tmp;
	}

	dirty_ = true;
}

Fs::File::File( Fs &fat, const char *fname )
	: fs_(fat)
{
	Directory *d = fs_.searchFile( fname );

	if ( d == NULL )
		fileNotFound( fname );

	chain_now_ = chain_top_ = d->chain;
	size_ = d->size;
	count_ = 0;

	pffs_top_ = fs_.dev_.getPffsTop();
}

void Fs::update()
{
	dev_.setAppStat( Device::APP_STOP );
	dev_.writeFlash( mblk_adr_, (char*)&master_block_, sizeof(master_block_), false );
	dev_.setAppStat( Device::APP_RUN );

	DEBUG_MSG("update size: %d\n", sizeof( master_block_ ));
	dirty_ = false;
}

inline int Fs::need_block( size_t filesize )
{
	return (filesize+(SECTOR_SIZE-1))/SECTOR_SIZE;
}

void Fs::makeChain( uint16_t *pchain, int blkcnt, int next )
{
	uint16_t *fat = master_block_.fat_chain;

	if ( blkcnt <= 0 ) {
		*pchain = FAT_END;
		return;
	}

	for ( int i=next; i<MAXFAT; i++ ) {
		if ( fat[i] == FAT_FREE ) {
			DEBUG_MSG("free no %d\n", i);
			*pchain = i;
			makeChain( &fat[i], blkcnt-1, i+1 );
			return;
		}
	}

	throw "internal error: \"chain error\"";
}

void Fs::createFile( const char *fname, size_t len )
{
	if ( strlen(fname)>FNAME_LEN ) {
		throw "file name is too long";
	}

	int req_blk = need_block( len );
	int free_blk = getFreeBlockCount();
	
	Directory *dir = searchFile( fname );

	if ( dir != NULL ) {
		int use_block = need_block( dir->size );
		free_blk += use_block;

		if ( req_blk>free_blk ) 
			throw "not enough space";

		removeFile( dir );			
	} else {
		if ( req_blk>free_blk )
			throw "not enough space";

		dir = freeDir();

		if ( dir == NULL )
			throw "too many files";
	}

	memset( dir, 0, sizeof(*dir) );
	std::strcpy( dir->name, fname );
	dir->size = len;

	DEBUG_MSG( "fname:%s len:%d\n", fname, len );

	makeChain( &dir->chain, req_blk, 1 );

	dirty_ = true;
}

size_t Fs::getFreeBlockCount()
{
	uint16_t *fat = master_block_.fat_chain;
	size_t free_cnt=0;
	for ( int i=0; i<MAXFAT; i++ ) {
		if ( fat[i]==FAT_FREE )
			free_cnt++;
	}

	return free_cnt;
}

Fs::~Fs( )
{
	if ( dirty_ )
		update();
}

void Fs::File::nextSector( )
{
	if ( ((count_+1) * 4096) > size_ )
		throw "pointer over in nextSector";

	count_++;

	chain_now_ = fs_.master_block_.fat_chain[chain_now_];
}

void Fs::File::readSector( char buf[], size_t len )
{
	fs_.dev_.readMem( pffs_top_+chain_now_*Fs::SECTOR_SIZE, buf, len );
}

void Fs::File::writeSector( const char buf[], size_t len )
{
	uint32_t adr = pffs_top_+chain_now_ * Fs::SECTOR_SIZE;

	fs_.dev_.eraseFlash( adr );
	fs_.dev_.writeFlash( adr, buf, len );

}

void Fs::File::download( std::FILE *out )
{
	int len = size_;
	char buf[Fs::SECTOR_SIZE];

	chain_now_=chain_top_;

	while ( true ) {
		int cpsize = len>Fs::SECTOR_SIZE? Fs::SECTOR_SIZE: len;

		readSector( buf, cpsize );
		std::fwrite( buf, 1, cpsize, out );

		len-=cpsize;

		if ( len<=0 )
			break;

		nextSector();
	}

	chain_now_ = chain_top_;
}

void Fs::File::upload( std::FILE *in )
{
	struct Lock {
		Fs &fs_;

		Lock( Fs &fs)
			:fs_(fs) {
			fs_.dev_.setAppStat( Device::APP_STOP );
		}
		~Lock( ) {
			fs_.dev_.setAppStat( Device::APP_RUN );
		}
	} lock( fs_ );

	char buf[Fs::SECTOR_SIZE];
	int len = size_;

	chain_now_ = chain_top_;

	while ( true ) {
		int cpsize = len>Fs::SECTOR_SIZE? Fs::SECTOR_SIZE: len;
		fread( buf, 1, cpsize, in );

		writeSector( buf, cpsize );

		len-=cpsize;

		if ( len<=0 )
			break;

		nextSector();
	}

	chain_now_=chain_top_;
}

void Fs::format()
{
	PffsMaster *m = &master_block_;

	m->ptr = 0x00c28004;
	m->resv = 0xffffffff;
	std::strcpy( m->signature, "PFFS Master Block" );

	memset( m->directory, 0, sizeof(m->directory) );
	memset( m->fat_chain, 0xff, sizeof(m->fat_chain) );
	m->fat_chain[0] = FAT_END;

	update();
}

}
}
