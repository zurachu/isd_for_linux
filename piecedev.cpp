#include "piecedev.h"
#include <stdint.h>
#include <cstring>
#include "debug.h"

#include <unistd.h>

#define USB_ID_PRODUCT 0x1000
#define USB_ID_VENDOR 0x0e19

#define PIECE_END_POINT_IN 0x82
#define PIECE_END_POINT_OUT 0x02
#define BULK_WAIT 500

namespace n {
namespace piece {

namespace {

class Init
{
public:
	Init() {
		usb_init();
		usb_find_busses();
		usb_find_devices();
	}
};

Init init;

}

UsbDevHandle::UsbDevHandle()
{
	struct usb_bus *busses;
	struct usb_bus *bus;

	busses = usb_get_busses();

	for ( bus=busses; bus; bus=bus->next ) {
		struct usb_device *dev;
		for ( dev = bus->devices; dev; dev = dev->next ) {
			if ( dev->descriptor.idProduct == USB_ID_PRODUCT && dev->descriptor.idVendor == USB_ID_VENDOR ) {

				usb_dev_handle *udev;
				udev = usb_open(dev);

				if ( udev == NULL )
					throw "open failed";

				usb_dev_ = udev;
				usb_config_ = dev->config;
				return;
			}
		}
	}

	throw "can't find device";
}

UsbDevHandle::~UsbDevHandle()
{
  usb_close(usb_dev_);
}

Device::Device()
	:mblk_adr_(pffs_top_)
{
	usb_config_descriptor *config = usb_dev_.config();

	if ( ::usb_set_configuration( usb_dev_, config->bConfigurationValue ) < 0 )
		throw ::usb_strerror();

	if ( ::usb_claim_interface( usb_dev_, config->interface->altsetting->bInterfaceNumber ) < 0 )
		throw ::usb_strerror();

	readVersion();
}

void Device::readVersion()
{
	char tmp[4];
	char info[32];

	tmp[0] = 0;
	tmp[1] = 32;

	write( tmp, 2 );
	read( info, 32 );

	uint32_t size = *((uint16_t*)(info));
	version_ = *((uint16_t*)(info+4));

	pffs_top_ = *((uint32_t*)(info+24));
	pffs_end_ = *((uint32_t*)(info+28));

	DEBUG_MSG( "size: %x version:%x top:%x end:%x\n", size, version_, pffs_top_, pffs_end_ );
}

void Device::write( const char *buf, size_t len, int timeout )
{
	if ( ::usb_bulk_write( usb_dev_, PIECE_END_POINT_OUT, const_cast<char*>(buf), len, timeout )<0 )
		throw ::usb_strerror();
}

void Device::read( char *buf, size_t len, int timeout )
{
	if ( ::usb_bulk_read( usb_dev_, PIECE_END_POINT_IN, buf, len, timeout )<0 )
		throw ::usb_strerror();
}

void Device::readMem( uint32_t begin, char *buf, size_t len )
{
	char tmp[10];
	tmp[0] = READ_MEM;
	*(uint32_t*)(tmp+1) = begin;
	*(uint32_t*)(tmp+5) = len;

	write( tmp, 9 );
	read( buf, len );
}

void Device::writeMem( uint32_t begin, const char *buf, size_t len )
{
	char tmp[10];
	tmp[0] = 3;
	*(uint32_t *)(tmp+1) = begin;
	*(uint32_t *)(tmp+5) = len;

	write( tmp, 9 );
	write( buf, len );
}

bool Device::doWriteFlash( uint32_t rom_adr, uint32_t buf_adr, size_t len )
{
	char tmp[16];

	tmp[0] = DO_FLASH_WRITE;
	*(uint32_t *)(tmp+1) = rom_adr;
	*(uint32_t *)(tmp+5) = buf_adr;
	*(uint32_t *)(tmp+9) = len;

	write( tmp, 13 );
	read( tmp, 2 );

	if ( (*(uint16_t*)tmp) == 0 )
		return true;
	return false;
}

bool Device::eraseFlash( uint32_t adr )
{
	char tmp[8];
	tmp[0] = ERASE_FLASH;
	*(uint32_t*)(tmp+1) = adr;

	write( tmp, 5 );
	read( tmp, 2 );

	if ( *(uint16_t*)tmp == 0 )
		return true;

	return false;
}

bool Device::writeFlash( uint32_t rom_adr, const char *buf, size_t len, bool safe )
{
	DEBUG_MSG( "rom_adr: %x, len: %d\n", rom_adr, len );

	if ( ( safe
	       && rom_adr<(pffs_top_+4096) )
	     || rom_adr>pffs_end_ ) {
		fprintf( stderr, "%x -> unsafe write flash\n", rom_adr);
		return false;
	}

	writeMem( 0x130000, buf, len );
	if ( eraseFlash( rom_adr ) == false )
		return false;
	bool ret = doWriteFlash( rom_adr, 0x130000, len );

	return ret;
}

namespace {

void milisleep( int mili ) {
	usleep( mili*1000 );
}

}

int Device::getAppStat( )
{
	char tmp[4];
	tmp[0] = GET_APPSTAT;

	write( tmp, 1 );
	read( tmp, 2 );

	return *(short*)tmp;
		
}

void Device::setAppStat( int stat )
{
	char tmp[4];
	tmp[0] = SET_APPSTAT;
	tmp[1] = stat;

	int next;

	if ( stat == 3 )
		next = 0;
	else if ( stat == 1 )
		next = 2;
	else
		return;

	write( tmp, 2 );

	for ( int i=0; i<100; i++ ) {
		int now;
		milisleep( 20 );
		now = getAppStat( );
		if ( now==next )
			return;
	}

	throw "appstat timeout";
}

}
}
