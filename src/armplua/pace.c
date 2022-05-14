#ifndef _PACE_C
#define _PACE_C

#include <PalmOS.h>
#include <VFSMgr.h>
#include "pealstub.h"

#define ARM

void write_unaligned32( unsigned char* dest, unsigned long val );
void write_unaligned16( unsigned char* dest, unsigned short val );

#define SWAP1_NON_NULL(ptr) if ( !!(ptr) ) { *(ptr) = Byte_Swap16(*(ptr)); }
#define SWAP2_NON_NULL(ptr) if ( !!(ptr) ) { *(ptr) = Byte_Swap16(*(ptr)); }
#define SWAP4_NON_NULL(ptr) if ( !!(ptr) ) { *(ptr) = Byte_Swap32(*(ptr)); }

#define STACK_SIZE(size) unsigned char stack[ size];
#define STACK_ADD32(par,offset) write_unaligned32( &stack[offset], (unsigned long )par )
#define STACK_ADD16(par,offset) write_unaligned16( &stack[offset], (unsigned short)par )
#define STACK_ADD8(val,offset) stack[offset] = (unsigned char)val;stack[offset+1] = 0
#define SET_SEL_REG(trap, sp) ((unsigned long*)(gEmulStateP))[3] = (trap)
#define STACK_RUN(trap,size) (*gCall68KFuncP)( gEmulStateP, PceNativeTrapNo(trap), stack, size )
#define STACK_RUN_RET_PTR(trap,size) (*gCall68KFuncP)( gEmulStateP, PceNativeTrapNo(trap), stack, kPceNativeWantA0 | size )

extern Int16 StrPrintF( Char* s, const Char* formatStr, ... );
extern Int16 StrVPrintF( Char* s, const Char* formatStr, _Palm_va_list arg );
extern Boolean SysHandleEvent( EventPtr eventP );
extern void FrmSetEventHandler( FormType* formP,FormEventHandlerType* handler );
extern void LstSetListChoices( ListType* listP, Char** itemsText,Int16 numItems );
extern Err SysNotifyRegister( UInt16 cardNo, LocalID dbID, 
			      UInt32 notifyType, SysNotifyProcPtr callbackP, 
			      Int8 priority, void* userDataP );
extern void LstSetDrawFunction( ListType* listP, ListDrawDataFuncPtr func );
extern Err ExgDBWrite( ExgDBWriteProcPtr writeProcP, void* userDataP, 
		       const char* nameP, LocalID dbID, UInt16 cardNo );


void evt68k2evtARM( EventType* event, const unsigned char* evt68k );
void evtArm2evt68K( unsigned char* evt68k, const EventType* event );
void flipRect( RectangleType* rout, const RectangleType* rin );
void flipFieldAttr( FieldAttrType* fout, const FieldAttrType* fin );
void flipEngSocketFromArm( unsigned char* sout, const ExgSocketType* sin );
void flipEngSocketToArm( ExgSocketType* out, const unsigned char* sin );
void flipFileInfoFromArm( unsigned char* fiout, const FileInfoType* fiin );
void flipFileInfoToArm( FileInfoType* fout, const unsigned char* fin );
void flipDateTimeToArm( DateTimeType* out, const unsigned char* in );

NetHostInfoPtr NetLibGetHostByName( UInt16 libRefNum, 
				    const Char* nameP, NetHostInfoBufPtr bufP, 
				    Int32 timeout, Err* errP );


#define crash()  *(int*)1L = 1

unsigned long Byte_Swap32( unsigned long l );
unsigned short Byte_Swap16( unsigned short l );
void write_unaligned16( unsigned char* dest, unsigned short val );
void write_unaligned32( unsigned char* dest, unsigned long val );
#define write_unaligned8( p, v ) *(p) = v

unsigned short read_unaligned16( const unsigned char* src );

void* memcpy( void* dest, const void* src, unsigned long n ){
	void* oldDest = dest;
	unsigned char* d = (unsigned char*)dest;
	unsigned char*s = (unsigned char*)src;
	if ( dest < src ) {
		while ( n-- > 0 ) *d++ = *s++;
	} else {
		d += n;
		s += n;
		while ( n-- > 0 ) *--d = *--s;
	}
	return oldDest;
} 

unsigned long Byte_Swap32( unsigned long l ){
	unsigned long result;
	result  = ((l >> 24) & 0xFF);
	result |= ((l >> 8) & 0xFF00);
	result |= ((l << 8) & 0xFF0000);
	result |= ((l << 24) & 0xFF000000);
	return result;
}

unsigned short Byte_Swap16( unsigned short l ){
	unsigned short result;
	result  = ((l >> 8) & 0xFF);
	result |= ((l << 8) & 0xFF00);
	return result;
}

void write_unaligned32( unsigned char* dest, unsigned long val ){
	unsigned int i;
	dest += sizeof(val);
	for ( i = 0; i < sizeof(val); ++i ) {
		*--dest = val & 0x000000FF;
		val >>= 8;
	}
} 

void write_unaligned16( unsigned char* dest, unsigned short val ){
	unsigned int i;
	dest += sizeof(val);
	for ( i = 0; i < sizeof(val); ++i ) {
		*--dest = val & 0x00FF;
		val >>= 8;
	}
} 

#define write_unaligned8( p, v ) *(p) = v

unsigned short read_unaligned16( const unsigned char* src ){
	unsigned int i;
	unsigned short val = 0;
	for ( i = 0; i < sizeof(val); ++i ) {
		val <<= 8;
		val |= *src++;
	}
	return val;
} 

#define read_unaligned8(cp) (*(cp))

unsigned long read_unaligned32( const unsigned char* src ){
	unsigned int i;
	unsigned long val = 0;
	for ( i = 0; i < sizeof(val); ++i ) {
		val <<= 8;
		val |= *src++;
	}
	return val;
} 


Int16 StrPrintF( Char* s, const Char* formatStr, ... ){
	unsigned long* inArgs = ((unsigned long*) (&formatStr)) + 1;
	return StrVPrintF( s, formatStr, (_Palm_va_list)inArgs );
} 


Int16 StrVPrintF( Char* s, const Char* formatStr, _Palm_va_list arg ){
	Int16 result;
	unsigned long* argv_arm = (unsigned long*)arg;
	unsigned char argv_68k[48];
	unsigned short done, isLong, innerDone, useArg;
	unsigned char* str = (unsigned char*)formatStr;
	unsigned short offset = 0;

	for ( done = 0; !done; ) {
		switch( *str++ ) {
			case '\0':
				done = 1; 
				break;
			case '%':
				isLong = useArg = 0;
				for( innerDone = 0; !innerDone; ) {
					unsigned char nxt = *str++;
					switch( nxt ) {                
						case '%':
							innerDone = 1;
							break;
						case 'l':
							isLong = 1;
							break;
						case 's':
							isLong = 1;
						case 'd':
						case 'x':
							innerDone = 1;
							useArg = 1;
							break;
						default:
							if ( nxt >= '0' && nxt <= '9' ) {
								
							} else {
								crash();
							}
					}
				}
				if ( useArg ) {
					unsigned long param;
					param = *argv_arm++;
					if ( isLong ) {
						write_unaligned32( &argv_68k[offset], param );
						offset += 4;
					} else {
						write_unaligned16( &argv_68k[offset],
								(unsigned short)param );
						offset += 2;
					}
				}
				break;
		}
	}

	
	{
		STACK_SIZE(12);
		STACK_ADD32(s, 0);
		STACK_ADD32(formatStr, 4);
		STACK_ADD32(argv_68k, 8);
		result = (Int16) STACK_RUN(sysTrapStrVPrintF,12);
	}

	return result;
} 

#define EVT_DATASIZE_68K 16 
void evt68k2evtARM( EventType* event, const unsigned char* evt68k ){
	event->eType = read_unaligned16( evt68k );
	event->penDown = read_unaligned8( evt68k+2 );
	event->tapCount = read_unaligned8( evt68k+3 );
	event->screenX = read_unaligned16( evt68k+4 );
	event->screenY = read_unaligned16( evt68k+6 );

	evt68k += 8;                

	switch ( event->eType ) {
		case frmLoadEvent:
		case frmOpenEvent:
		case frmCloseEvent:
		case frmUpdateEvent:
			event->data.frmLoad.formID = read_unaligned16( evt68k );
			event->data.frmUpdate.updateCode = read_unaligned16( evt68k + 2 );
			break;
		case keyDownEvent:
			event->data.keyDown.chr = read_unaligned16( evt68k );
			event->data.keyDown.keyCode = read_unaligned16( evt68k+2 );
			event->data.keyDown.modifiers = read_unaligned16( evt68k+4 );
			break;

		case ctlSelectEvent:
			event->data.ctlSelect.controlID = read_unaligned16(evt68k);
			event->data.ctlSelect.pControl
					= (ControlType*)read_unaligned32(evt68k+2);
			event->data.ctlSelect.on = (Boolean)read_unaligned8(evt68k+6);
			event->data.ctlSelect.reserved1 = read_unaligned8(evt68k+7);
			event->data.ctlSelect.value = read_unaligned16(evt68k+8);
			break;

		case winExitEvent:
		case winEnterEvent:
			event->data.winEnter.enterWindow
					= (WinHandle)read_unaligned32( evt68k );
			event->data.winEnter.exitWindow
					= (WinHandle)read_unaligned32( evt68k+4 );
			break;
		case menuEvent:
			event->data.menu.itemID = read_unaligned16( evt68k );
			break;
		case sclRepeatEvent:
			event->data.sclRepeat.scrollBarID = read_unaligned16( evt68k );
			event->data.sclRepeat.pScrollBar
					= (ScrollBarType*)read_unaligned32( evt68k+2 );
			event->data.sclRepeat.value = read_unaligned16( evt68k+6 );
			event->data.sclRepeat.newValue = read_unaligned16( evt68k+8 );
			event->data.sclRepeat.time = read_unaligned32( evt68k+10 );
			break;

			default:   
				memcpy( &event->data, evt68k, EVT_DATASIZE_68K );
				break;
	}
} 

void evtArm2evt68K( unsigned char* evt68k, const EventType* event ){
	write_unaligned16( evt68k, event->eType );
	write_unaligned8( evt68k + 2, event->penDown );
	write_unaligned8( evt68k + 3, event->tapCount );
	write_unaligned16( evt68k + 4, event->screenX );
	write_unaligned16( evt68k + 6, event->screenY );

	evt68k += 8;

	switch ( event->eType ) {
		case frmLoadEvent:
		case frmOpenEvent:
		case frmCloseEvent:
		case frmUpdateEvent:
			write_unaligned16( evt68k, event->data.frmLoad.formID );
			write_unaligned16( evt68k + 2, event->data.frmUpdate.updateCode );
			break;
		case keyDownEvent:
			write_unaligned16( evt68k, event->data.keyDown.chr );
			write_unaligned16( evt68k+2, event->data.keyDown.keyCode );
			write_unaligned16( evt68k+4, event->data.keyDown.modifiers );
			break;

		case ctlSelectEvent:
			write_unaligned16( evt68k, event->data.ctlSelect.controlID );
			write_unaligned32( evt68k+2, 
					   (unsigned long)event->data.ctlSelect.pControl );
			write_unaligned8( evt68k+6, event->data.ctlSelect.on );
			write_unaligned8( evt68k+7, event->data.ctlSelect.reserved1 );
			write_unaligned16( evt68k+8, event->data.ctlSelect.value );
			break;

		case winExitEvent:
		case winEnterEvent:
			write_unaligned32( evt68k, 
					   (unsigned long)event->data.winEnter.enterWindow );
			write_unaligned32( evt68k+4,
					   (unsigned long)event->data.winEnter.exitWindow );
			break;
		case menuEvent:
			write_unaligned16( evt68k, event->data.menu.itemID );
			break;
		case sclRepeatEvent:
			write_unaligned16( evt68k, event->data.sclRepeat.scrollBarID );
			write_unaligned32( evt68k+2, 
					   (unsigned long)event->data.sclRepeat.pScrollBar );
			write_unaligned16( evt68k+6, event->data.sclRepeat.value );
			write_unaligned16( evt68k+8, event->data.sclRepeat.newValue );
			write_unaligned32( evt68k+10, event->data.sclRepeat.time );
			break;
			

			default:   
				memcpy( evt68k, &event->data, EVT_DATASIZE_68K );
				break;
	}

} 


Boolean SysHandleEvent( EventPtr eventP ){
	Boolean result;
	EventType event68K;
	evtArm2evt68K( (unsigned char*)&event68K, eventP );
	{
		unsigned char stack[4];
		STACK_ADD32(&event68K, 0);
		result = STACK_RUN(sysTrapSysHandleEvent,4 );
	}
	return result;
} 

/* The stub wants to look like this:
   static Boolean
   FormEventHandlerType( EventType *eventP )
{
       unsigned long data[] = { armEvtHandler, eventP };
       return (Boolean)PceNativeCall( handlerEntryPoint, (void*)data );
}
 */
static unsigned char*
		makeHandlerStub( FormEventHandlerType* handlerArm )
{
	unsigned char* stub;
	unsigned char code_68k[] = {
        	0x4e, 0x56, 0xff, 0xf8,             // linkw %fp,#-8
        	0x20, 0x2e, 0x00, 0x08,         	// movel %fp@(8),%d0
        	0x2d, 0x7c, 0x11, 0x22, 0x33, 0x44, // movel #287454020,%fp@(-8)
         0xff, 0xf8,                         // ????? REQUIRED!!!!
        	0x2d, 0x40, 0xff, 0xfc,      	    // movel %d0,%fp@(-4)
        	0x48, 0x6e, 0xff, 0xf8,      	    // pea %fp@(-8)
        	0x2f, 0x3c, 0x55, 0x66, 0x77, 0x88, // movel #1432778632,%sp@-
        	0x4e, 0x4f,           	            // trap #15
        	0xa4, 0x5a,                         // 0122132
        	0x02, 0x40, 0x00, 0xff,      	    // andiw #255,%d0
        	0x4e, 0x5e,           	            // unlk %fp
        	0x4e, 0x75                          // rts
	};

	stub = MemPtrNew( sizeof(code_68k) );
	memcpy( stub, code_68k, sizeof(code_68k) );

	write_unaligned32( &stub[10],  (unsigned long)handlerArm );
	#define handlerEntryPoint 0
	write_unaligned32( &stub[26],  (unsigned long)handlerEntryPoint );
    return (unsigned char*)stub;
} 

void FrmSetEventHandler( FormType* formP, FormEventHandlerType* handler ){
	unsigned char* handlerStub = makeHandlerStub( handler );
	unsigned char stack[8];
	STACK_ADD32(formP, 0);
	STACK_ADD32(handlerStub, 4);
	
	STACK_RUN(sysTrapFrmSetEventHandler,8 );
} 

void flipRect( RectangleType* rout, const RectangleType* rin ){
	rout->topLeft.x = Byte_Swap16(rin->topLeft.x);
	rout->topLeft.y = Byte_Swap16(rin->topLeft.y);
	rout->extent.x = Byte_Swap16(rin->extent.x);
	rout->extent.y = Byte_Swap16(rin->extent.y);
} 

void flipFieldAttr( FieldAttrType* fout, const FieldAttrType* fin ){
} 

void flipEngSocketFromArm( unsigned char* sout, const ExgSocketType* sin ){
	write_unaligned16( &sout[0],  sin->libraryRef ); // UInt16 libraryRef;
	write_unaligned32( &sout[2],  sin->socketRef );  // UInt32 	socketRef;
	write_unaligned32( &sout[6],  sin->target );     // UInt32 	target;
	write_unaligned32( &sout[10], sin->count ); // UInt32	count;

	write_unaligned32( &sout[14], sin->length );// UInt32	length;

	write_unaligned32( &sout[18], sin->time );// UInt32	time;
	write_unaligned32( &sout[22], sin->appData );	// UInt32	appData;
	write_unaligned32( &sout[26], sin->goToCreator );	// UInt32 	goToCreator;
	write_unaligned16( &sout[30], sin->goToParams.dbCardNo );	// UInt16	goToParams.dbCardNo;
	write_unaligned32( &sout[32], sin->goToParams.dbID );	// LocalID	goToParams.dbID;
	write_unaligned16( &sout[36], sin->goToParams.recordNum );	// UInt16 	goToParams.recordNum;
	write_unaligned32( &sout[38], sin->goToParams.uniqueID );	// UInt32	goToParams.uniqueID;
	write_unaligned32( &sout[42], sin->goToParams.matchCustom );	// UInt32	goToParams.matchCustom;
    /* bitfield.  All we can do is copy the whole thing, assuming it's 16
	bits, and pray that no arm code wants to to use it. */
	write_unaligned16( &sout[46], *(UInt16*)((unsigned char*)&sin->goToParams.matchCustom) 
			+ sizeof(sin->goToParams.matchCustom) );
	write_unaligned32( &sout[48], (unsigned long)sin->description );	// Char *description;
	write_unaligned32( &sout[52], (unsigned long)sin->type );	// Char *type;
	write_unaligned32( &sout[56], (unsigned long)sin->name );	// Char *name;
} 

void flipEngSocketToArm( ExgSocketType* sout, const unsigned char* sin ){
	sout->libraryRef = read_unaligned16( &sin[0] );
	sout->socketRef = read_unaligned32( &sin[2] );
	sout->target = read_unaligned32( &sin[6] );
	sout->count = read_unaligned32( &sin[10] );
	sout->length = read_unaligned32( &sin[14] );
	sout->time = read_unaligned32( &sin[18] );
	sout->appData = read_unaligned32( &sin[22] );
	sout->goToCreator = read_unaligned32( &sin[26] );
	sout->goToParams.dbCardNo =  read_unaligned16( &sin[30] );
	sout->goToParams.dbID =  read_unaligned32( &sin[32] );
	sout->goToParams.recordNum = read_unaligned16( &sin[36] );
	sout->goToParams.uniqueID = read_unaligned32( &sin[38] );
	sout->goToParams.matchCustom =  read_unaligned32( &sin[42] );
    /* bitfield.  All we can do is copy the whole thing, assuming it's 16
	bits, and pray that no arm code wants to to use it. */
	*(UInt16*)(((unsigned char*)&sout->goToParams.matchCustom) 
			+ sizeof(sout->goToParams.matchCustom)) = read_unaligned16( &sin[46] );
	sout->description =  (Char*)read_unaligned32( &sin[48] );
	sout->type = (Char*)read_unaligned32( &sin[52] );
	sout->name = (Char*)read_unaligned32( &sin[56] );
} 

void flipFileInfoFromArm( unsigned char* fiout, const FileInfoType* fiin ){
	write_unaligned32( &fiout[0], fiin->attributes );
	write_unaligned32( &fiout[4], (unsigned long)fiin->nameP );
	write_unaligned16( &fiout[8], fiin->nameBufLen );
}

void flipFileInfoToArm( FileInfoType* fout, const unsigned char* fin ){
	fout->attributes = read_unaligned32( &fin[0] );
	fout->nameP = (Char*)read_unaligned32( &fin[4] );
	fout->nameBufLen = read_unaligned16( &fin[8] );
} 


void LstSetListChoices( ListType* listP, Char** itemsText, Int16 numItems ){
	UInt16 i;
	unsigned char stack[10];
	
	STACK_ADD32(listP, 0);
	STACK_ADD32(itemsText, 4);
	STACK_ADD16( numItems, 8);
	for ( i = 0; i < numItems; ++i ) {
		itemsText[i] = (Char*)Byte_Swap32( (unsigned long)itemsText[i] );
	}
	STACK_RUN(sysTrapLstSetListChoices,10 );
} 

#undef  sysNotifyVolumeMountedEvent
#undef  sysNotifyVolumeUnmountedEvent
#define sysNotifyVolumeMountedEvent 0x6D6C6F76
#define sysNotifyVolumeUnmountedEvent 0x756C6F76

static void params68KtoParamsArm( SysNotifyParamType* paramsArm, const unsigned char* params68K ){
	paramsArm->notifyType = read_unaligned32( &params68K[0] );
	paramsArm->broadcaster = read_unaligned32( &params68K[4] );
	paramsArm->notifyDetailsP = (void*)read_unaligned32( &params68K[8] );
	paramsArm->userDataP = (void*)read_unaligned32( &params68K[12] );
	paramsArm->handled = read_unaligned8( &params68K[16] );

    /* I don't do anything with the data passed in, so no need to swap it...
	But that'd change for others: make an ARM-corrected copy of the
	contents of notifyDetailsP if your handler will use it. */
	switch( paramsArm->notifyType ) {
		case (UInt32)sysNotifyVolumeUnmountedEvent:
		case (UInt32)sysNotifyVolumeMountedEvent:
			break;
#ifdef FEATURE_SILK
		case sysNotifyDisplayChangeEvent:
			break;
#endif
	}

} 

static void paramsArmtoParams68K( unsigned char* params68K, const SysNotifyParamType* armParams ){
	write_unaligned8( &params68K[16], armParams->handled );
} 

unsigned long notifyEntryPoint( const void* emulStateP,  void* userData68KP, Call68KFuncType* call68KFuncP ){
	unsigned long* data = (unsigned long*)userData68KP;
	SysNotifyProcPtr callback
			= (SysNotifyProcPtr)read_unaligned32( (unsigned char*)&data[0] );
	SysNotifyParamType armParams;
	unsigned long oldR10;
	unsigned char* params68K;
	Err result;
	asm( "mov %0, r10" : "=r" (oldR10) );
	params68K = (unsigned char*)read_unaligned32((unsigned char*)&data[1]);
	params68KtoParamsArm( &armParams, params68K );
	result = (*callback)(&armParams);
	paramsArmtoParams68K( params68K, &armParams );
	asm( "mov r10, %0" : : "r" (oldR10) );
	return (unsigned long)result;
} 

/* The stub wants to look like this:
   static Err SysNotifyProc(SysNotifyParamType *notifyParamsP) 
{
       unsigned long data[] = { armNotifyHandler, notifyParamsP };
       return (Err)PceNativeCall( handlerEntryPoint, (void*)data );
}
 */
static unsigned char* makeNotifyStub( SysNotifyProcPtr callback ){
	unsigned char* stub;
	unsigned char code_68k[] = {
        	0x4e, 0x56, 0xff, 0xf8,             // linkw %fp,#-8
        	0x20, 0x2e, 0x00, 0x08,         	// movel %fp@(8),%d0
        	0x2d, 0x7c, 0x11, 0x22, 0x33, 0x44, // movel #287454020,%fp@(-8)
         0xff, 0xf8,                         // ????? REQUIRED!!!!
        	0x2d, 0x40, 0xff, 0xfc,      	    // movel %d0,%fp@(-4)
        	0x48, 0x6e, 0xff, 0xf8,      	    // pea %fp@(-8)
        	0x2f, 0x3c, 0x55, 0x66, 0x77, 0x88, // movel #1432778632,%sp@-
        	0x4e, 0x4f,           	            // trap #15
        	0xa4, 0x5a,                         // 0122132
        	0x4e, 0x5e,           	            // unlk %fp
        	0x4e, 0x75                          // rts
	};
	stub = MemPtrNew( sizeof(code_68k) );
	memcpy( stub, code_68k, sizeof(code_68k) );
	write_unaligned32( &stub[10], (unsigned long)callback );
	write_unaligned32( &stub[26], (unsigned long)notifyEntryPoint );
    /* Need to register this stub so it can be freed (once leaking ceases to
	be ok on PalmOS) */
	return (unsigned char*)stub;
} 


Err SysNotifyRegister( UInt16 cardNo, LocalID dbID, UInt32 notifyType, SysNotifyProcPtr callbackP, Int8 priority, void* userDataP ){
	Err result;
	{
		unsigned char* handlerStub = makeNotifyStub( callbackP );
		unsigned char stack[20];
		
		STACK_ADD16( cardNo, 0);
		STACK_ADD32(dbID, 2);
		STACK_ADD32(notifyType, 6);
		STACK_ADD32(handlerStub, 10);
		STACK_ADD8( priority, 14);
		STACK_ADD32(userDataP, 16);
		
		result = (Err)STACK_RUN(sysTrapSysNotifyRegister,20 );
		
	}
     
	return result;
} 

unsigned long listDrawEntryPoint( const void* emulStateP, void* userData68KP, Call68KFuncType* call68KFuncP ){
	unsigned long* data = (unsigned long*)userData68KP;
	ListDrawDataFuncPtr listDrawProc = (ListDrawDataFuncPtr)read_unaligned32( (unsigned char*)&data[0] );
	unsigned long oldR10;
	Int16 index;
	char** itemsText;
	RectangleType rectArm;
	asm( "mov %0, r10" : "=r" (oldR10) );
	flipRect( &rectArm, (RectanglePtr)read_unaligned32( (unsigned char*)&data[2] ) );
	index = (Int16)read_unaligned32( (unsigned char*)&data[1] );
	itemsText = (char**)read_unaligned32( (unsigned char*)&data[3] );
	(*listDrawProc)( index, &rectArm, itemsText );
	asm( "mov r10, %0" : : "r" (oldR10) );
	return 0L;                  
} 

static unsigned char* makeListDrawStub( ListDrawDataFuncPtr func ){
/* called function looks like this:
	void listDrawFunc(Int16 index, RectanglePtr bounds, char** itemsText)
	{
	unsigned long data[] = { func, index, 
	bounds, itemsText };
	return (Err)PceNativeCall( listDrawEntryPoint, (void*)data );
}
 */
	unsigned char* stub;
	unsigned char code_68k[] = {
        	0x4e, 0x56, 0xff, 0xf0,      	// linkw %fp,#-16
        	0x30, 0x2e, 0x00, 0x08,      	// movew %fp@(8),%d0
        	0x22, 0x2e, 0x00, 0x0a,      	// movel %fp@(10),%d1
        	0x24, 0x2e, 0x00, 0x0e,      	// movel %fp@(14),%d2
        	0x2d, 0x7c, 0x11, 0x22,0x33,0x44,// movel #287454020,%fp@(-16)
        	0xff, 0xf0,
        	0x30, 0x40,           	// moveaw %d0,%a0
        	0x2d, 0x48, 0xff, 0xf4,      	// movel %a0,%fp@(-12)
        	0x2d, 0x41, 0xff, 0xf8,      	// movel %d1,%fp@(-8)
        	0x2d, 0x42, 0xff, 0xfc,      // movel %d2,%fp@(-4)
        	0x48, 0x6e, 0xff, 0xf0,      	// pea %fp@(-16)
        	0x2f, 0x3c, 0x55, 0x66, 0x77, 0x88,	// movel #1432778632,%sp@-
        	0x4e, 0x4f,           	// trap #15
        	0xa4, 0x5a,           	// 0122132
        	0x4e, 0x5e,           	// unlk %fp
        	0x4e, 0x75           	// rts
	};
	stub = MemPtrNew( sizeof(code_68k) );
	memcpy( stub, code_68k, sizeof(code_68k) );
	write_unaligned32( &stub[0x12], (unsigned long)func );
	write_unaligned32( &stub[0x2c], (unsigned long)listDrawEntryPoint );
	return (unsigned char*)stub;
} 


void LstSetDrawFunction( ListType* listP, ListDrawDataFuncPtr func ){
	unsigned char* stub = makeListDrawStub( func );
	unsigned char stack[8];
	
	STACK_ADD32(listP, 0);
	STACK_ADD32(stub, 4);
	STACK_RUN(sysTrapLstSetDrawFunction,8 );
} 

void flipDateTimeToArm( DateTimeType* out, const unsigned char* in ){
	const DateTimeType* inp = (DateTimeType*)in;
	out->second = Byte_Swap16( inp->second );
	out->minute = Byte_Swap16( inp->minute );
	out->hour = Byte_Swap16( inp->hour );
	out->day = Byte_Swap16( inp->day );
	out->month = Byte_Swap16( inp->month );
	out->year = Byte_Swap16( inp->year );
	out->weekDay = Byte_Swap16( inp->weekDay );
}


NetHostInfoPtr NetLibGetHostByName( UInt16 libRefNum, const Char* nameP,  NetHostInfoBufPtr bufP, Int32 timeout, Err* errP ){
	NetHostInfoPtr result;
	{
		unsigned char stack[18];
		STACK_ADD16( libRefNum, 0);
		STACK_ADD32(nameP, 2);
		STACK_ADD32(bufP, 6);
		STACK_ADD32(timeout, 10);
		STACK_ADD32(errP, 14);
		result = (NetHostInfoPtr) STACK_RUN_RET_PTR(netLibTrapGetHostByName, 18 );
		if ( result != NULL ) {
			result->nameP = (void*)Byte_Swap32( (UInt32)result->nameP );
			result->nameAliasesP = (void*)Byte_Swap32( (UInt32)result->nameAliasesP );
			result->addrType = Byte_Swap16( result->addrType );
			result->addrLen = Byte_Swap16( result->addrLen );
			result->addrListP = (void*)Byte_Swap32( (UInt32)result->addrListP );
			result->addrListP[0] = (void*)Byte_Swap32( (UInt32)result->addrListP[0] );
			*(UInt32*)result->addrListP[0] = Byte_Swap32( (UInt32)*(UInt32*)result->addrListP[0] );
		}
	}
	return result;
} 

static unsigned long exgWriteEntry( const void* emulStateP, void* userData68KP, Call68KFuncType* call68KFuncP ){
	unsigned long* data = (unsigned long*)userData68KP;
	unsigned long oldR10;
	ExgDBWriteProcPtr exgProc
			= (ExgDBWriteProcPtr)read_unaligned32( (unsigned char*)&data[0] );
	UInt32* sizeP;
	Err err;
	asm( "mov %0, r10" : "=r" (oldR10) );
	sizeP = (UInt32*)read_unaligned32( (unsigned char*)&data[2] );
	SWAP4_NON_NULL(sizeP);
	err = (*exgProc)( (const void*)read_unaligned32((unsigned char*)&data[1]), 
	sizeP, 
	(void*)read_unaligned32( (unsigned char*)&data[3] ) );
	SWAP4_NON_NULL(sizeP);

	asm( "mov r10, %0" : : "r" (oldR10) );

	return (unsigned long)err;                  
} 

static void makeExgWriteStub( ExgDBWriteProcPtr proc, unsigned char* stub, UInt16 stubSize ){
	unsigned char code_68k[] = {
		/* 0:*/	0x4e, 0x56, 0xff, 0xf0,      	/* linkw %fp,#-16 */
			/* 4:*/	0x20, 0x2e, 0x00, 0x08,      	/* movel %fp@(8),%d0 */
				/* 8:*/	0x22, 0x2e, 0x00, 0x0c,      	/* movel %fp@(12),%d1 */
					/* c:*/	0x24, 0x2e, 0x00, 0x10,      	/* movel %fp@(16),%d2 */
						/*10:*/	0x2d, 0x7c, 0x11, 0x22, 0x33, 0x44,/*movel #287454020,%fp@(-16)*/
        /*16:*/	0xff, 0xf0,
		/*18:*/	0x2d, 0x40, 0xff, 0xf4,      	/* movel %d0,%fp@(-12) */
			/*1c:*/	0x2d, 0x41, 0xff, 0xf8,      	/* movel %d1,%fp@(-8) */
				/*20:*/	0x2d, 0x42, 0xff, 0xfc,      	/* movel %d2,%fp@(-4) */
					/*24:*/	0x48, 0x6e, 0xff, 0xf0,      	/* pea %fp@(-16) */
						/*28:*/	0x2f, 0x3c, 0x55, 0x66, 0x77, 0x88,	/* movel #1432778632,%sp@- */
							/*2e:*/	0x4e, 0x4f,           	/* trap #15 */
								/*30:*/	0xa4, 0x5a,           	/* 0122132 */
									/*32:*/	0x4e, 0x5e,           	/* unlk %fp */
										/*34:*/	0x4e, 0x75           	/* rts */
	};
	
	memcpy( stub, code_68k, sizeof(code_68k) );

	write_unaligned32( &stub[0x12], 
			    
			    (unsigned long)proc );
	write_unaligned32( &stub[0x2a], 
			    
			    (unsigned long)exgWriteEntry );
} 


Err
		ExgDBWrite( ExgDBWriteProcPtr writeProcP, void* userDataP, 
			    const char* nameP, LocalID dbID, UInt16 cardNo )
{
	Err result;
     
	
	
	{
		unsigned char stub[0x36];
		unsigned char stack[18];
		makeExgWriteStub( writeProcP, stub, sizeof(stub) );

		
		STACK_ADD32(stub, 0);
		STACK_ADD32(userDataP, 4);
		STACK_ADD32(nameP, 8);
		STACK_ADD32(dbID, 12);
		STACK_ADD16( cardNo, 16);
		result = (Err)STACK_RUN(sysTrapExgDBWrite,18 );
		
	}
     
	return result;
} 


void
PrefSetAppPreferences( UInt32 creator, UInt16 id, Int16 version, const void* prefs, UInt16 prefsSize, Boolean saved )
{
	
 
	{

		STACK_SIZE(16);
 
		STACK_ADD32( creator, 0);
		STACK_ADD16( id, 4);
		STACK_ADD16( version, 6);
		STACK_ADD32( prefs, 8);
		STACK_ADD16( prefsSize, 12);
		STACK_ADD8 (saved, 14);
  
		STACK_RUN(  (sysTrapPrefSetAppPreferences), 16 );
 
	}
 
     
} 


void
		WinPushDrawState(  )
{
	{

		STACK_SIZE(0);
     
  
		STACK_RUN(  
				(sysTrapWinPushDrawState),
		0 );
 
	}
 
     
} 


MemHandle
		DmNewResource( DmOpenRef dbP, DmResType resType, DmResID resID, UInt32 size )
{
	MemHandle result;
 
 
     
	{

		STACK_SIZE( 14);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD32( resType, 4);
		STACK_ADD16( resID, 8);
		STACK_ADD32( size, 10);
  
		result = (MemHandle)STACK_RUN_RET_PTR( (sysTrapDmNewResource),14 );
 
	}
 
     
	return result;
} 


Err DmDatabaseInfo( UInt16 cardNo, LocalID dbID, Char* nameP, UInt16* attributesP, UInt16* versionP, UInt32* crDateP, UInt32* modDateP, UInt32* bckUpDateP, UInt32* modNumP, LocalID* appInfoIDP, LocalID* sortInfoIDP, UInt32* typeP, UInt32* creatorP )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(attributesP);
	SWAP2_NON_NULL(versionP);
	SWAP4_NON_NULL(crDateP);
	SWAP4_NON_NULL(modDateP);
	SWAP4_NON_NULL(bckUpDateP);
	SWAP4_NON_NULL(modNumP);
	SWAP4_NON_NULL(appInfoIDP);
	SWAP4_NON_NULL(sortInfoIDP);
	SWAP4_NON_NULL(typeP);
	SWAP4_NON_NULL(creatorP);
	{
		STACK_SIZE( 50);
     
		STACK_ADD16( cardNo, 0);
		STACK_ADD32( dbID, 2);
		STACK_ADD32( nameP, 6);
		STACK_ADD32( attributesP, 10);
		STACK_ADD32( versionP, 14);
		STACK_ADD32( crDateP, 18);
		STACK_ADD32( modDateP, 22);
		STACK_ADD32( bckUpDateP, 26);
		STACK_ADD32( modNumP, 30);
		STACK_ADD32( appInfoIDP, 34);
		STACK_ADD32( sortInfoIDP, 38);
		STACK_ADD32( typeP, 42);
		STACK_ADD32( creatorP, 46);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmDatabaseInfo),
		50 );
 
		SWAP2_NON_NULL(attributesP);
		SWAP2_NON_NULL(versionP);
		SWAP4_NON_NULL(crDateP);
		SWAP4_NON_NULL(modDateP);
		SWAP4_NON_NULL(bckUpDateP);
		SWAP4_NON_NULL(modNumP);
		SWAP4_NON_NULL(appInfoIDP);
		SWAP4_NON_NULL(sortInfoIDP);
		SWAP4_NON_NULL(typeP);
		SWAP4_NON_NULL(creatorP);
	}
 
     
	return result;
} 


Err
		VFSFileOpen( UInt16 volRefNum, const Char* pathNameP, UInt16 openMode, FileRef* fileRefP )
{
	Err result;
 
 
     
	SWAP4_NON_NULL(fileRefP);
	{

		STACK_SIZE( 12);
     
		STACK_ADD16( volRefNum, 0);
		STACK_ADD32( pathNameP, 2);
		STACK_ADD16( openMode, 6);
		STACK_ADD32( fileRefP, 8);
  
		SET_SEL_REG(vfsTrapFileOpen, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch),
		12 );
 
		SWAP4_NON_NULL(fileRefP);
	}
 
     
	return result;
} 


Char *  StrChr (const Char *str, WChar chr)
{
	Char* result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( str, 0);
		STACK_ADD16( chr, 4);
  
		result = (Char*)STACK_RUN_RET_PTR( (sysTrapStrChr),6 );
 
	}
 
     
	return result;
} 

Char*
		StrCopy( Char* dst, const Char* src )
{
	Char* result;
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( dst, 0);
		STACK_ADD32( src, 4);
  
		result = (Char*)STACK_RUN_RET_PTR( (sysTrapStrCopy),8 );
 
	}
 
     
	return result;
} 

Char*
		StrNCopy( Char* dst, const Char* src, Int16 n )
{
	Char* result;
 
 
     
	{

		STACK_SIZE( 10);
     
		STACK_ADD32( dst, 0);
		STACK_ADD32( src, 4);
		STACK_ADD16( n, 6);
  
		result = (Char*)STACK_RUN_RET_PTR( (sysTrapStrNCopy),10 );
 
	}
 
     
	return result;
} 

UInt16 TxtCharAttr(WChar inChar)
{
	UInt16 result;
 
 
     
	{

		STACK_SIZE( 2);
     
		STACK_ADD16( inChar, 0);
  
		result = (UInt16)STACK_RUN_RET_PTR( (intlTxtCharAttr),2 );
 
	}
 
     
	return result;
} 


MemHandle
		DmGetResourceIndex( DmOpenRef dbP, UInt16 index )
{
	MemHandle result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD16( index, 4);
  
		result = (MemHandle)STACK_RUN_RET_PTR( (sysTrapDmGetResourceIndex),6 );
 
	}
 
     
	return result;
} 


Err
		ExgRegisterData( UInt32 creatorID, UInt16 id, const Char* dataTypesP )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 10);
     
		STACK_ADD32( creatorID, 0);
		STACK_ADD16( id, 4);
		STACK_ADD32( dataTypesP, 6);
  
		result = (Err)STACK_RUN( 
				(sysTrapExgRegisterData),
		10 );
 
	}
 
     
	return result;
} 


IndexedColorType
		WinSetTextColor( IndexedColorType textColor )
{
	IndexedColorType result;
 
 
     
	{

		STACK_SIZE( 2);
     
		STACK_ADD8 (textColor, 0);
  
		result = (IndexedColorType)STACK_RUN( 
				(sysTrapWinSetTextColor),
		2 );
 
	}
 
     
	return result;
} 


Err
		FtrUnregister( UInt32 creator, UInt16 featureNum )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( creator, 0);
		STACK_ADD16( featureNum, 4);
  
		result = (Err)STACK_RUN( 
				(sysTrapFtrUnregister),
		6 );
 
	}
 
     
	return result;
} 


Err
		WinScreenGetAttribute( WinScreenAttrType selector, UInt32* attrP )
{
	Err result;
 
 
     
	SWAP4_NON_NULL(attrP);
	{

		STACK_SIZE( 6);
     
		STACK_ADD8 (selector, 0);
		STACK_ADD32( attrP, 2);
  
		SET_SEL_REG(HDSelectorWinScreenGetAttribute, sp);
		result = (Err)STACK_RUN( 
				(sysTrapHighDensityDispatch),
		6 );
 
		SWAP4_NON_NULL(attrP);
	}
 
     
	return result;
} 


void
		WinSetBounds( WinHandle winHandle, const RectangleType* rP )
{
 
 
	RectangleType RectangleType_68K1;
     
	flipRect( &RectangleType_68K1, rP );
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( winHandle, 0);
		STACK_ADD32( &RectangleType_68K1, 4);
  
		STACK_RUN(  
				(sysTrapWinSetBounds),
		8 );
 
	}
 
     
} 


Err
		DmWriteCheck( void* recordP, UInt32 offset, UInt32 bytes )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 12);
     
		STACK_ADD32( recordP, 0);
		STACK_ADD32( offset, 4);
		STACK_ADD32( bytes, 8);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmWriteCheck),
		12 );
 
	}
 
     
	return result;
} 


MemPtr
		MemHandleLock( MemHandle h )
{
	MemPtr result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( h, 0);
  
		result = (MemPtr)STACK_RUN_RET_PTR( (sysTrapMemHandleLock),4 );
 
	}
 
     
	return result;
} 


Char*
		StrCat( Char* dst, const Char* src )
{
	Char* result;
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( dst, 0);
		STACK_ADD32( src, 4);
  
		result = (Char*)STACK_RUN_RET_PTR((sysTrapStrCat),8 );
 
	}
 
     
	return result;
} 

Char*
		StrNCat( Char* dst, const Char* src, Int16 n )
{
	Char* result;
 
 
     
	{

		STACK_SIZE( 10);
     
		STACK_ADD32( dst, 0);
		STACK_ADD32( src, 4);
		STACK_ADD16( n, 6);
  
		result = (Char*)STACK_RUN_RET_PTR((sysTrapStrNCat),8 );
 
	}
 
     
	return result;
} 


WinHandle
		WinCreateOffscreenWindow( Coord width, Coord height, WindowFormatType format, UInt16* error )
{
	WinHandle result;
 
 
     
	SWAP2_NON_NULL(error);
	{

		STACK_SIZE( 10);
     
		STACK_ADD16( width, 0);
		STACK_ADD16( height, 2);
		STACK_ADD8 (format, 4);
		STACK_ADD32( error, 6);
  
		result = (WinHandle)STACK_RUN_RET_PTR( (sysTrapWinCreateOffscreenWindow),10 );
 
		SWAP2_NON_NULL(error);
	}
 
     
	return result;
} 


void
		WinDrawRectangleFrame( FrameType frame, const RectangleType* rP )
{
 
 
	RectangleType RectangleType_68K1;
     
	flipRect( &RectangleType_68K1, rP );
	{

		STACK_SIZE( 6);
     
		STACK_ADD16( frame, 0);
		STACK_ADD32( &RectangleType_68K1, 2);
  
		STACK_RUN(  
				(sysTrapWinDrawRectangleFrame),
		6 );
 
	}
 
     
} 


Err
		VFSDirEntryEnumerate( FileRef dirRef, UInt32* dirEntryIteratorP, FileInfoType* infoP )
{
	Err result;
 
 
	FileInfoType FileInfoType_68K2;
     
	SWAP4_NON_NULL(dirEntryIteratorP);
	flipFileInfoFromArm( (unsigned char*) &FileInfoType_68K2, infoP );
	{

		STACK_SIZE( 12);
     
		STACK_ADD32( dirRef, 0);
		STACK_ADD32( dirEntryIteratorP, 4);
		STACK_ADD32( &FileInfoType_68K2, 8);
  
		SET_SEL_REG(vfsTrapDirEntryEnumerate, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch),
		12 );
 
		SWAP4_NON_NULL(dirEntryIteratorP);
		flipFileInfoToArm( infoP, (void*)&FileInfoType_68K2 );
	}
 
     
	return result;
} 


Err
		MemHandleResize( MemHandle h, UInt32 newSize )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( h, 0);
		STACK_ADD32( newSize, 4);
  
		result = (Err)STACK_RUN( 
				(sysTrapMemHandleResize),
		8 );
 
	}
 
     
	return result;
} 


Err
		DmReleaseRecord( DmOpenRef dbP, UInt16 index, Boolean dirty )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD16( index, 4);
		STACK_ADD8 (dirty, 6);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmReleaseRecord),
		8 );
 
	}
 
     
	return result;
} 


Err
		DmReleaseResource( MemHandle resourceH )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( resourceH, 0);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmReleaseResource),
		4 );
 
	}
 
     
	return result;
} 


void
		TimSecondsToDateTime( UInt32 seconds, DateTimeType* dateTimeP )
{
 
 
	DateTimeType DateTimeType_68K1;
     
//	SWAP_DATETIMETYPE_ARM_TO_68K( &DateTimeType_68K1, dateTimeP );
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( seconds, 0);
		STACK_ADD32( &DateTimeType_68K1, 4);
  
		STACK_RUN(  
				(sysTrapTimSecondsToDateTime),
		8 );
 
		flipDateTimeToArm( dateTimeP, (void*)&DateTimeType_68K1 );
	}
 
     
} 


Err
		VFSFileDelete( UInt16 volRefNum, const Char* pathNameP )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD16( volRefNum, 0);
		STACK_ADD32( pathNameP, 2);
  
		SET_SEL_REG(vfsTrapFileDelete, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch),
		6 );
 
	}
 
     
	return result;
} 


Err
		VFSFileDBInfo( FileRef ref, Char* nameP, UInt16* attributesP, UInt16* versionP, UInt32* crDateP, UInt32* modDateP, UInt32* bckUpDateP, UInt32* modNumP, MemHandle* appInfoHP, MemHandle* sortInfoHP, UInt32* typeP, UInt32* creatorP, UInt16* numRecordsP )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(attributesP);
	SWAP2_NON_NULL(versionP);
	SWAP4_NON_NULL(crDateP);
	SWAP4_NON_NULL(modDateP);
	SWAP4_NON_NULL(bckUpDateP);
	SWAP4_NON_NULL(modNumP);
	SWAP4_NON_NULL(typeP);
	SWAP4_NON_NULL(creatorP);
	SWAP2_NON_NULL(numRecordsP);
	{

		STACK_SIZE( 52);
     
		STACK_ADD32( ref, 0);
		STACK_ADD32( nameP, 4);
		STACK_ADD32( attributesP, 8);
		STACK_ADD32( versionP, 12);
		STACK_ADD32( crDateP, 16);
		STACK_ADD32( modDateP, 20);
		STACK_ADD32( bckUpDateP, 24);
		STACK_ADD32( modNumP, 28);
		STACK_ADD32( appInfoHP, 32);
		STACK_ADD32( sortInfoHP, 36);
		STACK_ADD32( typeP, 40);
		STACK_ADD32( creatorP, 44);
		STACK_ADD32( numRecordsP, 48);
  
		SET_SEL_REG(vfsTrapFileDBInfo, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch),
		52 );
 
		SWAP2_NON_NULL(attributesP);
		SWAP2_NON_NULL(versionP);
		SWAP4_NON_NULL(crDateP);
		SWAP4_NON_NULL(modDateP);
		SWAP4_NON_NULL(bckUpDateP);
		SWAP4_NON_NULL(modNumP);
		SWAP4_NON_NULL(typeP);
		SWAP4_NON_NULL(creatorP);
		SWAP2_NON_NULL(numRecordsP);
	}
 
     
	return result;
} 


void
		WinDrawLine( Coord x1, Coord y1, Coord x2, Coord y2 )
{
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD16( x1, 0);
		STACK_ADD16( y1, 2);
		STACK_ADD16( x2, 4);
		STACK_ADD16( y2, 6);
  
		STACK_RUN(  
				(sysTrapWinDrawLine),
		8 );
 
	}
 
     
} 


void
		WinDrawBitmap( BitmapPtr bitmapP, Coord x, Coord y )
{
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( bitmapP, 0);
		STACK_ADD16( x, 4);
		STACK_ADD16( y, 6);
  
		STACK_RUN(  
				(sysTrapWinDrawBitmap),
		8 );
 
	}
 
     
} 


Int16
		FntCharsWidth( Char const* chars, Int16 len )
{
	Int16 result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( chars, 0);
		STACK_ADD16( len, 4);
  
		result = (Int16)STACK_RUN( 
				(sysTrapFntCharsWidth),
		6 );
 
	}
 
     
	return result;
} 


void
		WinInvertRectangle( const RectangleType* rP, UInt16 cornerDiam )
{
 
 
	RectangleType RectangleType_68K0;
     
	flipRect( &RectangleType_68K0, rP );
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( &RectangleType_68K0, 0);
		STACK_ADD16( cornerDiam, 4);
  
		STACK_RUN(  
				(sysTrapWinInvertRectangle),
		6 );
 
	}
 
     
} 


void
		WinDrawPixel( Coord x, Coord y )
{
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD16( x, 0);
		STACK_ADD16( y, 2);
  
		STACK_RUN(  
				(sysTrapWinDrawPixel),
		4 );
 
	}
 
     
} 


UInt16
		MemHandleLockCount( MemHandle h )
{
	UInt16 result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( h, 0);
  
		result = (UInt16)STACK_RUN( 
				(sysTrapMemHandleLockCount),
		4 );
 
	}
 
     
	return result;
} 


void
		WinSetClip( const RectangleType* rP )
{
 
 
	RectangleType RectangleType_68K0;
     
	flipRect( &RectangleType_68K0, rP );
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( &RectangleType_68K0, 0);
  
		STACK_RUN(  
				(sysTrapWinSetClip),
		4 );
 
	}
 
     
} 


void
		WinRestoreBits( WinHandle winHandle, Coord destX, Coord destY )
{
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( winHandle, 0);
		STACK_ADD16( destX, 4);
		STACK_ADD16( destY, 6);
  
		STACK_RUN(  
				(sysTrapWinRestoreBits),
		8 );
 
	}
 
     
} 


UInt16
		StrLen( const Char* src )
{
	UInt16 result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( src, 0);
  
		result = (UInt16)STACK_RUN( 
				(sysTrapStrLen),
		4 );
 
	}
 
     
	return result;
} 



Int16
		FntBaseLine(  )
{
	Int16 result;
 
 
     
	{

		STACK_SIZE( 0);
     
  
		result = (Int16)STACK_RUN( 
				(sysTrapFntBaseLine),
		0 );
 
	}
 
     
	return result;
} 


Err
		MemHandleUnlock( MemHandle h )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( h, 0);
  
		result = (Err)STACK_RUN( 
				(sysTrapMemHandleUnlock),
		4 );
 
	}
 
     
	return result;
} 


void
		WinEraseRectangle( const RectangleType* rP, UInt16 cornerDiam )
{
 
 
	RectangleType RectangleType_68K0;
     
	flipRect( &RectangleType_68K0, rP );
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( &RectangleType_68K0, 0);
		STACK_ADD16( cornerDiam, 4);
  
		STACK_RUN(  
				(sysTrapWinEraseRectangle),
		6 );
 
	}
 
     
} 


void
		WinEraseLine( Coord x1, Coord y1, Coord x2, Coord y2 )
{
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD16( x1, 0);
		STACK_ADD16( y1, 2);
		STACK_ADD16( x2, 4);
		STACK_ADD16( y2, 6);
  
		STACK_RUN(  
				(sysTrapWinEraseLine),
		8 );
 
	}
 
     
} 


void
		WinScrollRectangle( const RectangleType* rP, WinDirectionType direction, Coord distance, RectangleType* vacatedP )
{
 
 
	RectangleType RectangleType_68K0;
	RectangleType RectangleType_68K3;
     
	flipRect( &RectangleType_68K0, rP );
	flipRect( &RectangleType_68K3, vacatedP );
	{

		STACK_SIZE( 12);
     
		STACK_ADD32( &RectangleType_68K0, 0);
		STACK_ADD8 (direction, 4);
		STACK_ADD16( distance, 6);
		STACK_ADD32( &RectangleType_68K3, 8);
  
		STACK_RUN(  
				(sysTrapWinScrollRectangle),
		12 );
 
		flipRect( vacatedP, &RectangleType_68K3 );
	}
 
     
} 


Err
		VFSGetDefaultDirectory( UInt16 volRefNum, const Char* fileTypeStr, Char* pathStr, UInt16* bufLenP )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(bufLenP);
	{

		STACK_SIZE( 14);
     
		STACK_ADD16( volRefNum, 0);
		STACK_ADD32( fileTypeStr, 2);
		STACK_ADD32( pathStr, 6);
		STACK_ADD32( bufLenP, 10);
  
		SET_SEL_REG(vfsTrapGetDefaultDirectory, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch),
		14 );
 
		SWAP2_NON_NULL(bufLenP);
	}
 
     
	return result;
} 


Err
		FtrSet( UInt32 creator, UInt16 featureNum, UInt32 newValue )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 10);
     
		STACK_ADD32( creator, 0);
		STACK_ADD16( featureNum, 4);
		STACK_ADD32( newValue, 6);
  
		result = (Err)STACK_RUN( 
				(sysTrapFtrSet),
		10 );
 
	}
 
     
	return result;
} 


Char*
		StrIToA( Char* s, Int32 i )
{
	Char* result;
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( s, 0);
		STACK_ADD32( i, 4);
  
		result = (Char*)STACK_RUN_RET_PTR((sysTrapStrIToA),8 );
 
	}
 
     
	return result;
} 


void
		WinGetDrawWindowBounds( RectangleType* rP )
{
 
 
	RectangleType RectangleType_68K0;
     
	flipRect( &RectangleType_68K0, rP );
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( &RectangleType_68K0, 0);
  
		STACK_RUN(  
				(sysTrapWinGetDrawWindowBounds),
		4 );
 
		flipRect( rP, &RectangleType_68K0 );
	}
 
     
} 


Boolean
		PrefGetAppPreferencesV10( UInt32 type, Int16 version, void* prefs, UInt16 prefsSize )
{
	Boolean result;
 
 
     
	{

		STACK_SIZE( 12);
     
		STACK_ADD32( type, 0);
		STACK_ADD16( version, 4);
		STACK_ADD32( prefs, 6);
		STACK_ADD16( prefsSize, 10);
  
		result = (Boolean)STACK_RUN( 
				(sysTrapPrefGetAppPreferencesV10),
		12 );
 
	}
 
     
	return result;
} 


void
		WinEraseWindow(  )
{
 
 
     
	{

		STACK_SIZE( 0);
     
  
		STACK_RUN(  
				(sysTrapWinEraseWindow),
		0 );
 
	}
 
     
} 


Int16
		SysRandom( Int32 newSeed )
{
	Int16 result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( newSeed, 0);
  
		result = (Int16)STACK_RUN( 
				(sysTrapSysRandom),
		4 );
 
	}
 
     
	return result;
} 


Int32
		StrAToI( const Char* str )
{
	Int32 result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( str, 0);
  
		result = (Int32)STACK_RUN( 
				(sysTrapStrAToI),
		4 );
 
	}
 
     
	return result;
} 


UInt32
		ExgReceive( ExgSocketType* socketP, void* bufP, UInt32 bufLen, Err* err )
{
	UInt32 result;
 
 
	ExgSocketType ExgSocketType_68K0;
     
	flipEngSocketFromArm( (void*)&ExgSocketType_68K0, socketP );
	{

		STACK_SIZE( 16);
     
		STACK_ADD32( &ExgSocketType_68K0, 0);
		STACK_ADD32( bufP, 4);
		STACK_ADD32( bufLen, 8);
		STACK_ADD32( err, 12);
  
		result = (UInt32)STACK_RUN( 
				(sysTrapExgReceive),
		16 );
 
		flipEngSocketToArm( socketP, (void*)&ExgSocketType_68K0 );
	}
 
     
	return result;
} 


Int16
		StrNCompare( const Char* s1, const Char* s2, Int32 n )
{
	Int16 result;
 
 
     
	{

		STACK_SIZE( 12);
     
		STACK_ADD32( s1, 0);
		STACK_ADD32( s2, 4);
		STACK_ADD32( n, 8);
  
		result = (Int16)STACK_RUN( 
				(sysTrapStrNCompare),
		12 );
 
	}
 
     
	return result;
} 


MemHandle
		DmResizeRecord( DmOpenRef dbP, UInt16 index, UInt32 newSize )
{
	MemHandle result;
 
 
     
	{

		STACK_SIZE( 10);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD16( index, 4);
		STACK_ADD32( newSize, 6);
  
		result = (MemHandle)STACK_RUN_RET_PTR((sysTrapDmResizeRecord),10 );
 
	}
 
     
	return result;
} 


DmOpenRef
		DmOpenDatabase( UInt16 cardNo, LocalID dbID, UInt16 mode )
{
	DmOpenRef result;
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD16( cardNo, 0);
		STACK_ADD32( dbID, 2);
		STACK_ADD16( mode, 6);
  
		result = (DmOpenRef)STACK_RUN_RET_PTR((sysTrapDmOpenDatabase),8 );
 
	}
 
     
	return result;
} 

DmOpenRef
		DmOpenDBNoOverlay( UInt16 cardNo, LocalID dbID, UInt16 mode )
{
	DmOpenRef result;
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD16( cardNo, 0);
		STACK_ADD32( dbID, 2);
		STACK_ADD16( mode, 6);
  
		result = (DmOpenRef)STACK_RUN_RET_PTR((sysTrapDmOpenDBNoOverlay),8 );
 
	}
 
     
	return result;
} 


UInt32
		TimGetSeconds(  )
{
	UInt32 result;
 
 
     
	{

		STACK_SIZE( 0);
     
  
		result = (UInt32)STACK_RUN( 
				(sysTrapTimGetSeconds),
		0 );
 
	}
 
     
	return result;
} 


Err
		ExgPut( ExgSocketType* socketP )
{
	Err result;
 
 
	ExgSocketType ExgSocketType_68K0;
     
	flipEngSocketFromArm( (void*)&ExgSocketType_68K0, socketP );
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( &ExgSocketType_68K0, 0);
  
		result = (Err)STACK_RUN( 
				(sysTrapExgPut),
		4 );
 
		flipEngSocketToArm( socketP, (void*)&ExgSocketType_68K0 );
	}
 
     
	return result;
} 


WinHandle
		WinSaveBits( const RectangleType* source, UInt16* error )
{
	WinHandle result;
 
 
	RectangleType RectangleType_68K0;
     
	flipRect( &RectangleType_68K0, source );
	SWAP2_NON_NULL(error);
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( &RectangleType_68K0, 0);
		STACK_ADD32( error, 4);
  
		result = (WinHandle)STACK_RUN_RET_PTR( (sysTrapWinSaveBits),8 );
 
		SWAP2_NON_NULL(error);
	}
 
     
	return result;
} 


LocalID
		DmFindDatabase( UInt16 cardNo, const Char* nameP )
{
	LocalID result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD16( cardNo, 0);
		STACK_ADD32( nameP, 2);
  
		result = (LocalID)STACK_RUN( 
				(sysTrapDmFindDatabase),
		6 );
 
	}
 
     
	return result;
} 


void
		TimeToAscii( UInt8 hours, UInt8 minutes, TimeFormatType timeFormat, Char* pString )
{
 
 
     
	{

		STACK_SIZE( 10);
     
		STACK_ADD8 (hours, 0);
		STACK_ADD8 (minutes, 2);
		STACK_ADD8 (timeFormat, 4);
		STACK_ADD32( pString, 6);
  
		STACK_RUN(  
				(sysTrapTimeToAscii),
		10 );
 
	}
 
     
} 


Err
		DmGetNextDatabaseByTypeCreator( Boolean newSearch, DmSearchStatePtr stateInfoP, UInt32 type, UInt32 creator, Boolean onlyLatestVers, UInt16* cardNoP, LocalID* dbIDP )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(cardNoP);
	SWAP4_NON_NULL(dbIDP);
	{

		STACK_SIZE( 24);
     
		STACK_ADD8 (newSearch, 0);
		STACK_ADD32( stateInfoP, 2);
		STACK_ADD32( type, 6);
		STACK_ADD32( creator, 10);
		STACK_ADD8 (onlyLatestVers, 14);
		STACK_ADD32( cardNoP, 16);
		STACK_ADD32( dbIDP, 20);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmGetNextDatabaseByTypeCreator),
		24 );
 
		SWAP2_NON_NULL(cardNoP);
		SWAP4_NON_NULL(dbIDP);
	}
 
     
	return result;
} 


void
		WinGetDisplayExtent( Coord* extentX, Coord* extentY )
{
 
 
     
	SWAP2_NON_NULL(extentX);
	SWAP2_NON_NULL(extentY);
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( extentX, 0);
		STACK_ADD32( extentY, 4);
  
		STACK_RUN(  
				(sysTrapWinGetDisplayExtent),
		8 );
 
		SWAP2_NON_NULL(extentX);
		SWAP2_NON_NULL(extentY);
	}
 
     
} 


Err
		VFSFileClose( FileRef fileRef )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( fileRef, 0);
  
		SET_SEL_REG(vfsTrapFileClose, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch),
		4 );
 
	}
 
     
	return result;
} 


Err
		MemChunkFree( MemPtr chunkDataP )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( chunkDataP, 0);
  
		result = (Err)STACK_RUN( 
				(sysTrapMemChunkFree),
		4 );
 
	}
 
     
	return result;
} 


FontID
		FntGetFont(  )
{
	FontID result;
 
 
     
	{

		STACK_SIZE( 0);
     
  
		result = (FontID)STACK_RUN( 
				(sysTrapFntGetFont),
		0 );
 
	}
 
     
	return result;
} 


void
		WinFillRectangle( const RectangleType* rP, UInt16 cornerDiam )
{
 
 
	RectangleType RectangleType_68K0;
     
	flipRect( &RectangleType_68K0, rP );
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( &RectangleType_68K0, 0);
		STACK_ADD16( cornerDiam, 4);
  
		STACK_RUN(  
				(sysTrapWinFillRectangle),
		6 );
 
	}
 
     
} 


MemPtr
		MemPtrNew( UInt32 size )
{
	MemPtr result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( size, 0);
  
		result = (MemPtr)STACK_RUN_RET_PTR((sysTrapMemPtrNew),4 );
 
	}
 
     
	return result;
} 


Err
		SysLibFind( const Char* nameP, UInt16* refNumP )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(refNumP);
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( nameP, 0);
		STACK_ADD32( refNumP, 4);
  
		result = (Err)STACK_RUN( 
				(sysTrapSysLibFind),
		8 );
 
		SWAP2_NON_NULL(refNumP);
	}
 
     
	return result;
} 


Err
		DmSeekRecordInCategory( DmOpenRef dbP, UInt16* indexP, UInt16 offset, Int16 direction, UInt16 category )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(indexP);
	{

		STACK_SIZE( 14);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD32( indexP, 4);
		STACK_ADD16( offset, 8);
		STACK_ADD16( direction, 10);
		STACK_ADD16( category, 12);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmSeekRecordInCategory),
		14 );
 
		SWAP2_NON_NULL(indexP);
	}
 
     
	return result;
} 


Err
		MemSet( void* dstP, Int32 numBytes, UInt8 value )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 10);
     
		STACK_ADD32( dstP, 0);
		STACK_ADD32( numBytes, 4);
		STACK_ADD8 (value, 8);
  
		result = (Err)STACK_RUN( 
				(sysTrapMemSet),
		10 );
 
	}
 
     
	return result;
} 


Int16
		NetLibSelect( UInt16 libRefNum, UInt16 width, NetFDSetType* readFDs, NetFDSetType* writeFDs, NetFDSetType* exceptFDs, Int32 timeout, Err* errP )
{
	Int16 result;
 
 
     
	SWAP4_NON_NULL(readFDs);
	SWAP4_NON_NULL(writeFDs);
	SWAP4_NON_NULL(exceptFDs);
	{

		STACK_SIZE( 24);
     
		STACK_ADD16( libRefNum, 0);
		STACK_ADD16( width, 2);
		STACK_ADD32( readFDs, 4);
		STACK_ADD32( writeFDs, 8);
		STACK_ADD32( exceptFDs, 12);
		STACK_ADD32( timeout, 16);
		STACK_ADD32( errP, 20);
  
		result = (Int16)STACK_RUN( 
				(netLibTrapSelect),
		24 );
 
		SWAP4_NON_NULL(readFDs);
		SWAP4_NON_NULL(writeFDs);
		SWAP4_NON_NULL(exceptFDs);
	}
 
     
	return result;
} 


Err
		ExgAccept( ExgSocketType* socketP )
{
	Err result;
 
 
	ExgSocketType ExgSocketType_68K0;
     
	flipEngSocketFromArm( (void*)&ExgSocketType_68K0, socketP );
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( &ExgSocketType_68K0, 0);
  
		result = (Err)STACK_RUN( 
				(sysTrapExgAccept),
		4 );
 
		flipEngSocketToArm( socketP, (void*)&ExgSocketType_68K0 );
	}
 
     
	return result;
} 


UInt16
		SysTicksPerSecond(  )
{
	UInt16 result;
 
 
     
	{

		STACK_SIZE( 0);
     
  
		result = (UInt16)STACK_RUN( 
				(sysTrapSysTicksPerSecond),
		0 );
 
	}
 
     
	return result;
} 



UInt32 MemPtrSize( MemPtr p )
{
	UInt32 result;
	{
		STACK_SIZE( 4);
		STACK_ADD32( p, 0);
		result = (UInt32)STACK_RUN( (sysTrapMemPtrSize),4 );
	}
	return result;
}


IndexedColorType
		WinSetBackColor( IndexedColorType backColor )
{
	IndexedColorType result;
 
 
     
	{

		STACK_SIZE( 2);
     
		STACK_ADD8 (backColor, 0);
  
		result = (IndexedColorType)STACK_RUN( 
				(sysTrapWinSetBackColor),
		2 );
 
	}
 
     
	return result;
} 


Int16
		FntLineHeight(  )
{
	Int16 result;
	{
		STACK_SIZE( 0);
		result = (Int16)STACK_RUN( 
				(sysTrapFntLineHeight),
		0 );
	}
	return result;
} 


Int16 StrCompare( const Char* s1, const Char* s2 )
{
	Int16 result;
	{
		STACK_SIZE( 8);
		STACK_ADD32( s1, 0);
		STACK_ADD32( s2, 4);
		result = (Int16)STACK_RUN( (sysTrapStrCompare),8 ); 
	}
	return result;
} 


MemHandle
		DmGetRecord( DmOpenRef dbP, UInt16 index )
{
	MemHandle result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD16( index, 4);
  
		result = (MemHandle)STACK_RUN_RET_PTR((sysTrapDmGetRecord),6 );
 
	}
 
     
	return result;
} 


void
		WinPopDrawState(  )
{
 
 
     
	{

		STACK_SIZE( 0);
     
  
		STACK_RUN(  
				(sysTrapWinPopDrawState),
		0 );
 
	}
 
     
} 


Boolean
		EvtSysEventAvail( Boolean ignorePenUps )
{
	Boolean result;
 
 
     
	{

		STACK_SIZE( 2);
     
		STACK_ADD8 (ignorePenUps, 0);
  
		result = (Boolean)STACK_RUN( 
				(sysTrapEvtSysEventAvail),
		2 );
 
	}
 
     
	return result;
} 


Int16
		NetLibSocketConnect( UInt16 libRefnum, NetSocketRef socket, NetSocketAddrType* sockAddrP, Int16 addrLen, Int32 timeout, Err* errP )
{
	Int16 result;
 
 
     
	{

		STACK_SIZE( 18);
     
		STACK_ADD16( libRefnum, 0);
		STACK_ADD16( socket, 2);
		STACK_ADD32( sockAddrP, 4);
		STACK_ADD16( addrLen, 8);
		STACK_ADD32( timeout, 10);
		STACK_ADD32( errP, 14);
  
		result = (Int16)STACK_RUN( 
				(netLibTrapSocketConnect),
		18 );
 
	}
 
     
	return result;
} 


Err
		SysUIAppSwitch( UInt16 cardNo, LocalID dbID, UInt16 cmd, MemPtr cmdPBP )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 12);
     
		STACK_ADD16( cardNo, 0);
		STACK_ADD32( dbID, 2);
		STACK_ADD16( cmd, 6);
		STACK_ADD32( cmdPBP, 8);
  
		result = (Err)STACK_RUN( 
				(sysTrapSysUIAppSwitch),
		12 );
 
	}
 
     
	return result;
} 


Int16
		NetLibSocketClose( UInt16 libRefnum, NetSocketRef socket, Int32 timeout, Err* errP )
{
	Int16 result;
 
 
     
	{

		STACK_SIZE( 12);
     
		STACK_ADD16( libRefnum, 0);
		STACK_ADD16( socket, 2);
		STACK_ADD32( timeout, 4);
		STACK_ADD32( errP, 8);
  
		result = (Int16)STACK_RUN( 
				(netLibTrapSocketClose),
		12 );
 
	}
 
     
	return result;
} 


Char*
		StrStr( const Char* str, const Char* token )
{
	Char* result;
 
 
     
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( str, 0);
		STACK_ADD32( token, 4);
  
		result = (Char*)STACK_RUN_RET_PTR( (sysTrapStrStr),8 );
 
	}
 
     
	return result;
} 


Err
		DmCloseDatabase( DmOpenRef dbP )
{
	Err result;
	{
		STACK_SIZE( 4);
		STACK_ADD32( dbP, 0);
		result = (Err)STACK_RUN( (sysTrapDmCloseDatabase),4 );
 
	}
 
     
	return result;
} 


void
		WinDrawRectangle( const RectangleType* rP, UInt16 cornerDiam )
{
 
 
	RectangleType RectangleType_68K0;
     
	flipRect( &RectangleType_68K0, rP );
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( &RectangleType_68K0, 0);
		STACK_ADD16( cornerDiam, 4);
  
		STACK_RUN(  
				(sysTrapWinDrawRectangle),
		6 );
 
	}
 
     
} 


Int16
		NetLibSocketOptionSet( UInt16 libRefnum, NetSocketRef socket, UInt16 level, UInt16 option, void* optValueP, UInt16 optValueLen, Int32 timeout, Err* errP )
{
	Int16 result;
 
 
     
	{

		STACK_SIZE( 22);
     
		STACK_ADD16( libRefnum, 0);
		STACK_ADD16( socket, 2);
		STACK_ADD16( level, 4);
		STACK_ADD16( option, 6);
		STACK_ADD32( optValueP, 8);
		STACK_ADD16( optValueLen, 12);
		STACK_ADD32( timeout, 14);
		STACK_ADD32( errP, 18);
  
		result = (Int16)STACK_RUN( 
				(netLibTrapSocketOptionSet),
		22 );
 
	}
 
     
	return result;
} 


Boolean
		RctPtInRectangle( Coord x, Coord y, const RectangleType* rP )
{
	Boolean result;
 
 
	RectangleType RectangleType_68K2;
     
	flipRect( &RectangleType_68K2, rP );
	{

		STACK_SIZE( 8);
     
		STACK_ADD16( x, 0);
		STACK_ADD16( y, 2);
		STACK_ADD32( &RectangleType_68K2, 4);
  
		result = (Boolean)STACK_RUN( 
				(sysTrapRctPtInRectangle),
		8 );
 
	}
 
     
	return result;
} 


Err
		NetLibClose( UInt16 libRefnum, UInt16 immediate )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD16( libRefnum, 0);
		STACK_ADD16( immediate, 2);
  
		result = (Err)STACK_RUN( 
				(sysLibTrapClose),
		4 );
 
	}
 
     
	return result;
} 


UInt32
		ExgSend( ExgSocketType* socketP, const void* bufP, UInt32 bufLen, Err* err )
{
	UInt32 result;
 
 
	ExgSocketType ExgSocketType_68K0;
     
	flipEngSocketFromArm( (void*)&ExgSocketType_68K0, socketP );
	{

		STACK_SIZE( 16);
     
		STACK_ADD32( &ExgSocketType_68K0, 0);
		STACK_ADD32( bufP, 4);
		STACK_ADD32( bufLen, 8);
		STACK_ADD32( err, 12);
  
		result = (UInt32)STACK_RUN( 
				(sysTrapExgSend),
		16 );
 
		flipEngSocketToArm( socketP, (void*)&ExgSocketType_68K0 );
	}
 
     
	return result;
} 


UInt32
		TimGetTicks(  )
{
	UInt32 result;
 
 
     
	{

		STACK_SIZE( 0);
     
  
		result = (UInt32)STACK_RUN( 
				(sysTrapTimGetTicks),
		0 );
 
	}
 
     
	return result;
} 


WinHandle
		WinSetDrawWindow( WinHandle winHandle )
{
	WinHandle result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( winHandle, 0);
  
		result = (WinHandle)STACK_RUN_RET_PTR( (sysTrapWinSetDrawWindow),4 );
 
	}
 
     
	return result;
} 


Err
		VFSExportDatabaseToFile( UInt16 volRefNum, const Char* pathNameP, UInt16 cardNo, LocalID dbID )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 12);
     
		STACK_ADD16( volRefNum, 0);
		STACK_ADD32( pathNameP, 2);
		STACK_ADD16( cardNo, 6);
		STACK_ADD32( dbID, 8);
  
		SET_SEL_REG(vfsTrapExportDatabaseToFile, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch),
		12 );
 
	}
 
     
	return result;
} 


Err
		DmWrite( void* recordP, UInt32 offset, const void* srcP, UInt32 bytes )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 16);
     
		STACK_ADD32( recordP, 0);
		STACK_ADD32( offset, 4);
		STACK_ADD32( srcP, 8);
		STACK_ADD32( bytes, 12);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmWrite),
		16 );
 
	}
 
     
	return result;
} 


IndexedColorType
		WinRGBToIndex( const RGBColorType* rgbP )
{
	IndexedColorType result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( rgbP, 0);
  
		result = (IndexedColorType)STACK_RUN( 
				(sysTrapWinRGBToIndex),
		4 );
 
	}
 
     
	return result;
} 


void
		WinSetPattern( const CustomPatternType* patternP )
{
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( patternP, 0);
  
		STACK_RUN(  
				(sysTrapWinSetPattern),
		4 );
 
	}
 
     
} 


UInt16
		DmNumRecordsInCategory( DmOpenRef dbP, UInt16 category )
{
	UInt16 result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD16( category, 4);
  
		result = (UInt16)STACK_RUN( 
				(sysTrapDmNumRecordsInCategory),
		6 );
 
	}
 
     
	return result;
} 


Err
		ExgDisconnect( ExgSocketType* socketP, Err error )
{
	Err result;
 
 
	ExgSocketType ExgSocketType_68K0;
     
	flipEngSocketFromArm( (void*)&ExgSocketType_68K0, socketP );
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( &ExgSocketType_68K0, 0);
		STACK_ADD16( error, 4);
  
		result = (Err)STACK_RUN( 
				(sysTrapExgDisconnect),
		6 );
 
		flipEngSocketToArm( socketP, (void*)&ExgSocketType_68K0 );
	}
 
     
	return result;
} 


Err
		WinScreenMode( WinScreenModeOperation operation, UInt32* widthP, UInt32* heightP, UInt32* depthP, Boolean* enableColorP )
{
	Err result;
 
 
     
	SWAP4_NON_NULL(widthP);
	SWAP4_NON_NULL(heightP);
	SWAP4_NON_NULL(depthP);
	SWAP1_NON_NULL(enableColorP);
	{

		STACK_SIZE( 18);
     
		STACK_ADD8 (operation, 0);
		STACK_ADD32( widthP, 2);
		STACK_ADD32( heightP, 6);
		STACK_ADD32( depthP, 10);
		STACK_ADD32( enableColorP, 14);
  
		result = (Err)STACK_RUN( 
				(sysTrapWinScreenMode),
		18 );
 
		SWAP4_NON_NULL(widthP);
		SWAP4_NON_NULL(heightP);
		SWAP4_NON_NULL(depthP);
		SWAP1_NON_NULL(enableColorP);
	}
 
     
	return result;
} 


NetSocketRef
		NetLibSocketOpen( UInt16 libRefnum, NetSocketAddrEnum domain, NetSocketTypeEnum type, Int16 protocol, Int32 timeout, Err* errP )
{
	NetSocketRef result;
 
 
     
	{

		STACK_SIZE( 16);
     
		STACK_ADD16( libRefnum, 0);
		STACK_ADD8 (domain, 2);
		STACK_ADD8 (type, 4);
		STACK_ADD16( protocol, 6);
		STACK_ADD32( timeout, 8);
		STACK_ADD32( errP, 12);
  
		result = (NetSocketRef)STACK_RUN( 
				(netLibTrapSocketOpen),
		16 );
 
	}
 
     
	return result;
} 


void
		WinDrawChars( const Char* chars, Int16 len, Coord x, Coord y )
{
 
 
     
	{

		STACK_SIZE( 10);
     
		STACK_ADD32( chars, 0);
		STACK_ADD16( len, 4);
		STACK_ADD16( x, 6);
		STACK_ADD16( y, 8);
  
		STACK_RUN(  
				(sysTrapWinDrawChars),
		10 );
 
	}
}
 
void WinPaintChars( const Char* chars, Int16 len, Coord x, Coord y )
{
 
 
     
	{

		STACK_SIZE( 10);
     
		STACK_ADD32( chars, 0);
		STACK_ADD16( len, 4);
		STACK_ADD16( x, 6);
		STACK_ADD16( y, 8);
  
		STACK_RUN(  
				(sysTrapWinPaintChars),
		10 );
 
	}
	
} 


Err
		FtrPtrFree( UInt32 creator, UInt16 featureNum )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( creator, 0);
		STACK_ADD16( featureNum, 4);
  
		result = (Err)STACK_RUN( 
				(sysTrapFtrPtrFree),
		6 );
 
	}
 
     
	return result;
} 


Err
		DmDeleteDatabase( UInt16 cardNo, LocalID dbID )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD16( cardNo, 0);
		STACK_ADD32( dbID, 2);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmDeleteDatabase),
		6 );
 
	}
 
     
	return result;
} 


Err
		VFSImportDatabaseFromFile( UInt16 volRefNum, const Char* pathNameP, UInt16* cardNoP, LocalID* dbIDP )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(cardNoP);
	SWAP4_NON_NULL(dbIDP);
	{

		STACK_SIZE( 14);
     
		STACK_ADD16( volRefNum, 0);
		STACK_ADD32( pathNameP, 2);
		STACK_ADD32( cardNoP, 6);
		STACK_ADD32( dbIDP, 10);
  
		SET_SEL_REG(vfsTrapImportDatabaseFromFile, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch),
		14 );
 
		SWAP2_NON_NULL(cardNoP);
		SWAP4_NON_NULL(dbIDP);
	}
 
     
	return result;
} 


void
		WinEraseRectangleFrame( FrameType frame, const RectangleType* rP )
{
 
 
	RectangleType RectangleType_68K1;
     
	flipRect( &RectangleType_68K1, rP );
	{

		STACK_SIZE( 6);
     
		STACK_ADD16( frame, 0);
		STACK_ADD32( &RectangleType_68K1, 2);
  
		STACK_RUN(  
				(sysTrapWinEraseRectangleFrame),
		6 );
 
	}
 
     
} 


MemHandle
		DmNewRecord( DmOpenRef dbP, UInt16* atP, UInt32 size )
{
	MemHandle result;
 
 
     
	SWAP2_NON_NULL(atP);
	{

		STACK_SIZE( 12);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD32( atP, 4);
		STACK_ADD32( size, 8);
  
		result = (MemHandle)STACK_RUN_RET_PTR( (sysTrapDmNewRecord),12 );
 
		SWAP2_NON_NULL(atP);
	}
 
     
	return result;
} 


FontID
		FntSetFont( FontID font )
{
	FontID result;
 
 
     
	{

		STACK_SIZE( 2);
     
		STACK_ADD8 (font, 0);
  
		result = (FontID)STACK_RUN( 
				(sysTrapFntSetFont),
		2 );
 
	}
 
     
	return result;
} 


UInt32
		WinSetScalingMode( UInt32 mode )
{
	UInt32 result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( mode, 0);
  
		SET_SEL_REG(HDSelectorWinSetScalingMode, sp);
		result = (UInt32)STACK_RUN( 
				(sysTrapHighDensityDispatch),
		4 );
 
	}
 
     
	return result;
} 


IndexedColorType
		WinSetForeColor( IndexedColorType foreColor )
{
	IndexedColorType result;
 
 
     
	{

		STACK_SIZE( 2);
     
		STACK_ADD8 (foreColor, 0);
  
		result = (IndexedColorType)STACK_RUN( 
				(sysTrapWinSetForeColor),
		2 );
 
	}
 
     
	return result;
} 


MemHandle
		DmGetResource( DmResType type, DmResID resID )
{
	MemHandle result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( type, 0);
		STACK_ADD16( resID, 4);
  
		result = (MemHandle)STACK_RUN_RET_PTR( (sysTrapDmGetResource),6 );
 
	}
 
     
	return result;
} 


void
		WinDeleteWindow( WinHandle winHandle, Boolean eraseIt )
{
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( winHandle, 0);
		STACK_ADD8 (eraseIt, 4);
  
		STACK_RUN(  
				(sysTrapWinDeleteWindow),
		6 );
 
	}
 
     
} 


Int16
		NetLibReceive( UInt16 libRefNum, NetSocketRef socket, void* bufP, UInt16 bufLen, UInt16 flags, void* fromAddrP, UInt16* fromLenP, Int32 timeout, Err* errP )
{
	Int16 result;
 
 
     
	SWAP2_NON_NULL(fromLenP);
	{

		STACK_SIZE( 28);
     
		STACK_ADD16( libRefNum, 0);
		STACK_ADD16( socket, 2);
		STACK_ADD32( bufP, 4);
		STACK_ADD16( bufLen, 8);
		STACK_ADD16( flags, 10);
		STACK_ADD32( fromAddrP, 12);
		STACK_ADD32( fromLenP, 16);
		STACK_ADD32( timeout, 20);
		STACK_ADD32( errP, 24);
  
		result = (Int16)STACK_RUN( 
				(netLibTrapReceive),
		28 );
 
		SWAP2_NON_NULL(fromLenP);
	}
 
     
	return result;
} 


Err VFSVolumeEnumerate( UInt16* volRefNumP, UInt32* volIteratorP ){
	Err result;
	SWAP2_NON_NULL(volRefNumP);
	SWAP4_NON_NULL(volIteratorP);
	{
		STACK_SIZE( 8);
		STACK_ADD32( volRefNumP, 0);
		STACK_ADD32( volIteratorP, 4);
		SET_SEL_REG(vfsTrapVolumeEnumerate, sp);
		
		result = (Err)STACK_RUN((sysTrapFileSystemDispatch),8 );
 
		SWAP2_NON_NULL(volRefNumP);
		SWAP4_NON_NULL(volIteratorP);
	}
	return result;
} 


Err
		DmSetDatabaseInfo( UInt16 cardNo, LocalID dbID, const Char* nameP, UInt16* attributesP, UInt16* versionP, UInt32* crDateP, UInt32* modDateP, UInt32* bckUpDateP, UInt32* modNumP, LocalID* appInfoIDP, LocalID* sortInfoIDP, UInt32* typeP, UInt32* creatorP )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(attributesP);
	SWAP2_NON_NULL(versionP);
	SWAP4_NON_NULL(crDateP);
	SWAP4_NON_NULL(modDateP);
	SWAP4_NON_NULL(bckUpDateP);
	SWAP4_NON_NULL(modNumP);
	SWAP4_NON_NULL(appInfoIDP);
	SWAP4_NON_NULL(sortInfoIDP);
	SWAP4_NON_NULL(typeP);
	SWAP4_NON_NULL(creatorP);
	{

		STACK_SIZE( 50);
     
		STACK_ADD16( cardNo, 0);
		STACK_ADD32( dbID, 2);
		STACK_ADD32( nameP, 6);
		STACK_ADD32( attributesP, 10);
		STACK_ADD32( versionP, 14);
		STACK_ADD32( crDateP, 18);
		STACK_ADD32( modDateP, 22);
		STACK_ADD32( bckUpDateP, 26);
		STACK_ADD32( modNumP, 30);
		STACK_ADD32( appInfoIDP, 34);
		STACK_ADD32( sortInfoIDP, 38);
		STACK_ADD32( typeP, 42);
		STACK_ADD32( creatorP, 46);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmSetDatabaseInfo),
		50 );
 
		SWAP2_NON_NULL(attributesP);
		SWAP2_NON_NULL(versionP);
		SWAP4_NON_NULL(crDateP);
		SWAP4_NON_NULL(modDateP);
		SWAP4_NON_NULL(bckUpDateP);
		SWAP4_NON_NULL(modNumP);
		SWAP4_NON_NULL(appInfoIDP);
		SWAP4_NON_NULL(sortInfoIDP);
		SWAP4_NON_NULL(typeP);
		SWAP4_NON_NULL(creatorP);
	}
 
     
	return result;
} 


Err
		DmRemoveRecord( DmOpenRef dbP, UInt16 index )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD16( index, 4);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmRemoveRecord),
		6 );
 
	}
 
     
	return result;
} 


Err
		DmGetLastErr(  )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 0);
     
  
		result = (Err)STACK_RUN( 
				(sysTrapDmGetLastErr),
		0 );
 
	}
 
     
	return result;
} 


UInt16
		WinSetCoordinateSystem( UInt16 coordSys )
{
	UInt16 result;
 
 
     
	{

		STACK_SIZE( 2);
     
		STACK_ADD16( coordSys, 0);
  
		SET_SEL_REG(HDSelectorWinSetCoordinateSystem, sp);
		result = (UInt16)STACK_RUN( 
				(sysTrapHighDensityDispatch),
		2 );
 
	}
 
     
	return result;
} 


Err
		MemPtrUnlock( MemPtr p )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( p, 0);
  
		result = (Err)STACK_RUN( 
				(sysTrapMemPtrUnlock),
		4 );
 
	}
 
     
	return result;
} 


UInt16
		DmFindResource( DmOpenRef dbP, DmResType resType, DmResID resID, MemHandle resH )
{
	UInt16 result;
 
 
     
	{

		STACK_SIZE( 14);
     
		STACK_ADD32( dbP, 0);
		STACK_ADD32( resType, 4);
		STACK_ADD16( resID, 8);
		STACK_ADD32( resH, 10);
  
		result = (UInt16)STACK_RUN( 
				(sysTrapDmFindResource),
		14 );
 
	}
 
     
	return result;
} 


Err
		NetLibOpen( UInt16 libRefnum, UInt16* netIFErrsP )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(netIFErrsP);
	{

		STACK_SIZE( 6);
     
		STACK_ADD16( libRefnum, 0);
		STACK_ADD32( netIFErrsP, 2);
  
		result = (Err)STACK_RUN( 
				(sysLibTrapOpen),
		6 );
 
		SWAP2_NON_NULL(netIFErrsP);
	}
 
     
	return result;
} 


WinHandle
		WinGetDrawWindow(  )
{
	WinHandle result;
	{
		STACK_SIZE( 0);
		result = (WinHandle)STACK_RUN_RET_PTR( (sysTrapWinGetDrawWindow),0 );
	}
	return result;
}

Err
		MemMove( void* dstP, const void* sP, Int32 numBytes )
{
	Err result;
	{
		STACK_SIZE( 12);
		STACK_ADD32( dstP, 0);
		STACK_ADD32( sP, 4);
		STACK_ADD32( numBytes, 8);
		result = (Err)STACK_RUN( 
				(sysTrapMemMove),
		12 );
	}
	return result;
} 

Int16 MemCmp (const void *s1, const void *s2, Int32 numBytes)
{
	Int16 result;
	{
		STACK_SIZE( 12);
		STACK_ADD32( s1, 0);
		STACK_ADD32( s2, 4);
		STACK_ADD32( numBytes, 8);
		result = (Err)STACK_RUN( 
				(sysTrapMemCmp),
		12 );
	}
	return result;
} 


void
		WinCopyRectangle( WinHandle srcWin, WinHandle dstWin, const RectangleType* srcRect, Coord destX, Coord destY, WinDrawOperation mode )
{
	RectangleType RectangleType_68K2;
	flipRect( &RectangleType_68K2, srcRect );
	{
		STACK_SIZE( 18);
		STACK_ADD32( srcWin, 0);
		STACK_ADD32( dstWin, 4);
		STACK_ADD32( &RectangleType_68K2, 8);
		STACK_ADD16( destX, 12);
		STACK_ADD16( destY, 14);
		STACK_ADD8 (mode, 16);
		STACK_RUN(  
				(sysTrapWinCopyRectangle),
		18 );
 	}
 
     
} 


MemHandle
		DmQueryRecord( DmOpenRef dbP, UInt16 index )
{
	MemHandle result;
	{
		STACK_SIZE( 6);
		STACK_ADD32( dbP, 0);
		STACK_ADD16( index, 4);
		result = (MemHandle)STACK_RUN_RET_PTR( (sysTrapDmQueryRecord),6 );
	}
 
     
	return result;
} 


void
		WinGetClip( RectangleType* rP )
{
 
 
	RectangleType RectangleType_68K0;
     
	flipRect( &RectangleType_68K0, rP );
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( &RectangleType_68K0, 0);
  
		STACK_RUN(  
				(sysTrapWinGetClip),
		4 );
 
		flipRect( rP, &RectangleType_68K0 );
	}
 
     
} 


UInt16
		DmNumRecords( DmOpenRef dbP )
{
	UInt16 result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( dbP, 0);
  
		result = (UInt16)STACK_RUN( 
				(sysTrapDmNumRecords),
		4 );
 
	}
 
     
	return result;
} 


Err
		FtrGet( UInt32 creator, UInt16 featureNum, UInt32* valueP )
{
	Err result;
 
 
     
	SWAP4_NON_NULL(valueP);
	{

		STACK_SIZE( 10);
     
		STACK_ADD32( creator, 0);
		STACK_ADD16( featureNum, 4);
		STACK_ADD32( valueP, 6);
  
		result = (Err)STACK_RUN( 
				(sysTrapFtrGet),
		10 );
 
		SWAP4_NON_NULL(valueP);
	}
 
     
	return result;
} 


Int16
		NetLibSend( UInt16 libRefNum, NetSocketRef socket, void* bufP, UInt16 bufLen, UInt16 flags, void* toAddrP, UInt16 toLen, Int32 timeout, Err* errP )
{
	Int16 result;
 
 
     
	{

		STACK_SIZE( 26);
     
		STACK_ADD16( libRefNum, 0);
		STACK_ADD16( socket, 2);
		STACK_ADD32( bufP, 4);
		STACK_ADD16( bufLen, 8);
		STACK_ADD16( flags, 10);
		STACK_ADD32( toAddrP, 12);
		STACK_ADD16( toLen, 16);
		STACK_ADD32( timeout, 18);
		STACK_ADD32( errP, 22);
  
		result = (Int16)STACK_RUN( 
				(netLibTrapSend),
		26 );
 
	}
 
     
	return result;
} 


Int16
		PrefGetAppPreferences( UInt32 creator, UInt16 id, void* prefs, UInt16* prefsSize, Boolean saved )
{
	Int16 result;
 
 
     
	SWAP2_NON_NULL(prefsSize);
	{

		STACK_SIZE( 16);
     
		STACK_ADD32( creator, 0);
		STACK_ADD16( id, 4);
		STACK_ADD32( prefs, 6);
		STACK_ADD32( prefsSize, 10);
		STACK_ADD8 (saved, 14);
  
		result = (Int16)STACK_RUN( 
				(sysTrapPrefGetAppPreferences),
		16 );
 
		SWAP2_NON_NULL(prefsSize);
	}
 
     
	return result;
} 


Err
		GrfSetState( Boolean capsLock, Boolean numLock, Boolean upperShift )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 6);
     
		STACK_ADD8 (capsLock, 0);
		STACK_ADD8 (numLock, 2);
		STACK_ADD8 (upperShift, 4);
  
		result = (Err)STACK_RUN( 
				(sysTrapGrfSetState),
		6 );
 
	}
 
     
	return result;
} 


Err
		SysLibLoad( UInt32 libType, UInt32 libCreator, UInt16* refNumP )
{
	Err result;
 
 
     
	SWAP2_NON_NULL(refNumP);
	{

		STACK_SIZE( 12);
     
		STACK_ADD32( libType, 0);
		STACK_ADD32( libCreator, 4);
		STACK_ADD32( refNumP, 8);
  
		result = (Err)STACK_RUN( 
				(sysTrapSysLibLoad),
		12 );
 
		SWAP2_NON_NULL(refNumP);
	}
 
     
	return result;
} 


UInt32
		MemHandleSize( MemHandle h )
{
	UInt32 result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( h, 0);
  
		result = (UInt32)STACK_RUN( 
				(sysTrapMemHandleSize),
		4 );
 
	}
 
     
	return result;
} 


void
		RctGetIntersection( const RectangleType* r1P, const RectangleType* r2P, RectangleType* r3P )
{
 
 
	RectangleType RectangleType_68K0;
	RectangleType RectangleType_68K1;
	RectangleType RectangleType_68K2;
     
	flipRect( &RectangleType_68K0, r1P );
	flipRect( &RectangleType_68K1, r2P );
	flipRect( &RectangleType_68K2, r3P );
	{

		STACK_SIZE( 12);
     
		STACK_ADD32( &RectangleType_68K0, 0);
		STACK_ADD32( &RectangleType_68K1, 4);
		STACK_ADD32( &RectangleType_68K2, 8);
  
		STACK_RUN(  
				(sysTrapRctGetIntersection),
		12 );
 
		flipRect( r3P, &RectangleType_68K2 );
	}
 
     
} 


Err
		SysNotifyUnregister( UInt16 cardNo, LocalID dbID, UInt32 notifyType, Int8 priority )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 12);
     
		STACK_ADD16( cardNo, 0);
		STACK_ADD32( dbID, 2);
		STACK_ADD32( notifyType, 6);
		STACK_ADD8 (priority, 10);
  
		result = (Err)STACK_RUN( 
				(sysTrapSysNotifyUnregister),
		12 );
 
	}
 
     
	return result;
} 


MemHandle
		MemPtrRecoverHandle( MemPtr p )
{
	MemHandle result;
 
 
     
	{

		STACK_SIZE( 4);
     
		STACK_ADD32( p, 0);
  
		result = (MemHandle)STACK_RUN_RET_PTR( (sysTrapMemPtrRecoverHandle),4 );
 
	}
 
     
	return result;
} 


UInt16
		DmFindResourceType( DmOpenRef dbP, DmResType resType, UInt16 typeIndex )
{
	UInt16 result;
	{
		STACK_SIZE( 10);
		STACK_ADD32( dbP, 0);
		STACK_ADD32( resType, 4);
		STACK_ADD16( typeIndex, 8);
		result = (UInt16)STACK_RUN((sysTrapDmFindResourceType),10);
	}
	return result;
} 


Err
		DmCreateDatabase( UInt16 cardNo, const Char* nameP, UInt32 creator, UInt32 type, Boolean resDB )
{
	Err result;
 
 
     
	{

		STACK_SIZE( 16);
     
		STACK_ADD16( cardNo, 0);
		STACK_ADD32( nameP, 2);
		STACK_ADD32( creator, 6);
		STACK_ADD32( type, 10);
		STACK_ADD8 (resDB, 14);
  
		result = (Err)STACK_RUN( 
				(sysTrapDmCreateDatabase),
		16 );
 
	}
 
     
	return result;
} 


void
		DateToAscii( UInt8 months, UInt8 days, UInt16 years, DateFormatType dateFormat, Char* pString )
{
 
 
     
	{

		STACK_SIZE( 12);
     
		STACK_ADD8 (months, 0);
		STACK_ADD8 (days, 2);
		STACK_ADD16( years, 4);
		STACK_ADD8 (dateFormat, 6);
		STACK_ADD32( pString, 8);
  
		STACK_RUN(  
				(sysTrapDateToAscii),
		12 );
 
	}
 
     
} 

void
		EvtGetEvent( EventType* event, Int32 timeout )
{
	 
	
	EventType EventType_68K0;
	 
	evtArm2evt68K( (void*)&EventType_68K0, event );
	{
		
		STACK_SIZE(8);
		 
		STACK_ADD32( (void*)&EventType_68K0, 0);
		STACK_ADD32( timeout, 4);
		
		STACK_RUN( 
				(sysTrapEvtGetEvent),
		8 );
		
		evt68k2evtARM( event, (void*)&EventType_68K0 );
	}
	 
	 
} 


void
		EvtGetPen( Int16* pScreenX, Int16* pScreenY, Boolean* pPenDown )
{
	 
	
	 
	
	{
		
		STACK_SIZE(12);
		 
		STACK_ADD32( pScreenX, 0);
		STACK_ADD32( pScreenY, 4);
		STACK_ADD32( pPenDown, 8);
		
		STACK_RUN( 
				(sysTrapEvtGetPen),
		12 );
		
		SWAP2_NON_NULL(pScreenX);
		SWAP2_NON_NULL(pScreenY);
	}
	 
	 
} 

void     BmpGetDimensions (const BitmapType * bitmapP, Coord * widthP, Coord * heightP, UInt16 * rowBytesP)
{
	{
		
		STACK_SIZE(16);
		 
		STACK_ADD32( bitmapP, 0);
		STACK_ADD32( widthP, 4);
		STACK_ADD32( heightP, 8);
		STACK_ADD32( rowBytesP, 12);
		
		STACK_RUN((sysTrapBmpGetDimensions),16 );
		
		SWAP2_NON_NULL(widthP);
		SWAP2_NON_NULL(heightP);
		SWAP2_NON_NULL(rowBytesP);
	}
	 
	 
}

		
		
void*   BmpGetBits (BitmapType * bitmapP){
	void* result;
	{
		STACK_SIZE( 4);
		STACK_ADD32( bitmapP, 0);
		result = (void* )STACK_RUN_RET_PTR( (sysTrapBmpGetBits),4 );
	}
	return result;

}

Err SndPlayResource (SndPtr soundP, Int32 volume,UInt32 flags)
{
	Err result;
	{

		STACK_SIZE( 12);
     
		STACK_ADD32( soundP, 0);
		STACK_ADD32( volume, 4);
		STACK_ADD32( flags, 8);
  
		result = (Err)STACK_RUN( 
				(sysTrapSndPlayResource),
		12 );
 
	}
 
     
	return result;
}



Err KeyRates (Boolean set, UInt16* initDelayP,
	      UInt16* periodP, UInt16* doubleTapDelayP,
	      Boolean* queueAheadP)
{
	Err result;
	{

		STACK_SIZE( 18);
     
		STACK_ADD8( set, 0);
		STACK_ADD32( initDelayP, 2);
		STACK_ADD32( periodP, 6);
		STACK_ADD32( doubleTapDelayP, 10);
		STACK_ADD32( queueAheadP, 14);
 
		result = (Err)STACK_RUN( 
				(sysTrapKeyRates),
		18 );
 
	}
 
     
	return result;
}

UInt32 KeyCurrentState (void)
{
	UInt32 result;
	{

		STACK_SIZE( 0);
		result = (UInt32)STACK_RUN( 
				(sysTrapKeyCurrentState),
		0 );

	}
 
     
	return result;
}
		
UInt32 KeySetMask (UInt32 keyMask)
{
	UInt32 result;
	{
		STACK_SIZE( 4);
		STACK_ADD32( keyMask, 0);
		result = (UInt32)STACK_RUN((sysTrapKeySetMask), 4 );
	}
	return result;
}


Err EvtFlushKeyQueue ()
{
	{
		STACK_SIZE( 0);
		STACK_RUN( (sysTrapEvtFlushKeyQueue),0 );
	}
	return 0;
}
		
Err SysTaskDelay (Int32 delay)
{
	Err result;
	{
		STACK_SIZE( 4);
		STACK_ADD32( delay, 0);
		result = (UInt32)STACK_RUN( (sysTrapSysTaskDelay),4 );

	}
 
     
	return result;
}

Err SysSetOrientation(UInt16 orientation){
	Err result;
	{
		STACK_SIZE( 2);
		STACK_ADD16( orientation, 0);
		SET_SEL_REG(pinSysSetOrientation, sp);
		result = (Err)STACK_RUN((sysTrapPinsDispatch),2 );
	}
	return result;
}

UInt16 SysGetOrientation(){
	UInt16 result;
	{
		STACK_SIZE( 0);
		SET_SEL_REG(pinSysGetOrientation, sp);
		result = (UInt16)STACK_RUN((sysTrapPinsDispatch),0 );
	}
	return result;
}

		
UInt16 PINGetInputAreaState(){
	UInt16 result;
	{
		STACK_SIZE( 0);
		SET_SEL_REG(pinPINGetInputAreaState, sp);
		result = (UInt16)STACK_RUN((sysTrapPinsDispatch),0 );
	}
	return result;
}

Err PINSetInputTriggerState(UInt16 state){
	Err result;
	{
		STACK_SIZE( 2);
		STACK_ADD16( state, 0);
		SET_SEL_REG(pinPINSetInputTriggerState, sp);
		result = (Err)STACK_RUN((sysTrapPinsDispatch),2 );
	}
	return result;
}

Err PINSetInputAreaState(UInt16 state){
	Err result;
	{
		STACK_SIZE( 2);
		STACK_ADD16( state, 0);
		SET_SEL_REG(pinPINSetInputAreaState, sp);
		result = (Err)STACK_RUN((sysTrapPinsDispatch),2 );
	}
	return result;
}

Err SysSetOrientationTriggerState(UInt16 triggerState){
	Err result;
	{
		STACK_SIZE( 2);
		STACK_ADD16( triggerState, 0);
		SET_SEL_REG(pinSysSetOrientationTriggerState, sp);
		result = (Err)STACK_RUN((sysTrapPinsDispatch),2 );
	}
	return result;
}

Err StatHide(){
	Err result;
	{
		STACK_SIZE( 0);
		SET_SEL_REG(pinStatHide, sp);
		result = (Err)STACK_RUN((sysTrapPinsDispatch),0 );
	}
	return result;
}

Err StatShow(){
	Err result;
	{
		STACK_SIZE( 0);
		SET_SEL_REG(pinStatShow, sp);
		result = (Err)STACK_RUN((sysTrapPinsDispatch),0 );
	}
	return result;
}


Err StatGetAttribute(UInt16 selector, UInt32* dataP){
	Err result;
	{
		STACK_SIZE( 6);
		SWAP4_NON_NULL(dataP);
		STACK_ADD16( selector, 0);
		STACK_ADD32( dataP, 2);
		SET_SEL_REG(pinStatGetAttribute, sp);
		result = (Err)STACK_RUN((sysTrapPinsDispatch),6 );
		SWAP4_NON_NULL(dataP);
	}
	return result;
}

Err VskSetState(UInt16 refNum, UInt16 stateType, UInt16 state)
{
	Err result;
	{

		STACK_SIZE( 6);
		STACK_ADD16( refNum, 0);
		STACK_ADD16( stateType, 2);
		STACK_ADD16( state, 4);
		result = (Err)STACK_RUN((0xA80B), 6 );

	}
	return result;
}


UInt32 VskGetAPIVersion(UInt16 refNum)
{
	UInt32 result;
	{

		STACK_SIZE( 2);
		STACK_ADD16( refNum, 0);
		result = (UInt32)STACK_RUN((0xA808), 2 );

	}
	return result;
}

Err VskOpen(UInt16 refNum)
{
	Err result;
	{

		STACK_SIZE( 2);
		STACK_ADD16( refNum, 0);
		result = (Err)STACK_RUN((sysLibTrapOpen), 2 );

	}
	return result;
}

Err VskClose(UInt16 refNum)
{
	Err result;
	{

		STACK_SIZE( 2);
		STACK_ADD16( refNum, 0);
		result = (Err)STACK_RUN((sysLibTrapClose), 2 );

	}
	return result;
}

UInt8 *WinScreenLock (WinLockInitType initMode){
	UInt8* result;
	{
		STACK_SIZE( 2);
		STACK_ADD8( initMode, 0);
		result = (UInt8*)STACK_RUN_RET_PTR( (sysTrapWinScreenLock),2 );
 	}
 	return result;
}

void WinScreenUnlock (){
	{
		STACK_SIZE( 0);
		STACK_RUN( (sysTrapWinScreenUnlock),0 );
	}
}

BitmapType *WinGetBitmap (WinHandle winHandle){
	BitmapType * result;
	{
		STACK_SIZE( 4);
		STACK_ADD32( winHandle, 0);
		result = (BitmapType*)STACK_RUN_RET_PTR( (sysTrapWinGetBitmap),4 );
	}
	return result;
}

Err VFSFileSeek(FileRef fileRef, FileOrigin origin, Int32 offset){
	Err result;
	{

		STACK_SIZE( 10);
		STACK_ADD32( fileRef, 0);
		STACK_ADD16( origin, 4);
		STACK_ADD32( offset, 6);
		SET_SEL_REG(vfsTrapFileSeek, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch), 10);
	}
	return result;
}

Err VFSFileRead(FileRef fileRef, UInt32 numBytes, void *bufP, UInt32 *numBytesReadP){
	Err result;
	{

		STACK_SIZE( 16);
		STACK_ADD32( fileRef, 0);
		STACK_ADD32( numBytes, 4);
		STACK_ADD32( bufP, 8);
		STACK_ADD32( numBytesReadP, 12);
		SET_SEL_REG(vfsTrapFileRead, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch), 16);
	}
	return result;
}

Err VFSFileWrite(FileRef fileRef, UInt32 numBytes, const void *dataP, UInt32 *numBytesWrittenP){
	Err result;
	{

		STACK_SIZE( 16);
		STACK_ADD32( fileRef, 0);
		STACK_ADD32( numBytes, 4);
		STACK_ADD32( dataP, 8);
		STACK_ADD32( numBytesWrittenP, 12);
		SET_SEL_REG(vfsTrapFileWrite, sp);
		result = (Err)STACK_RUN( 
				(sysTrapFileSystemDispatch), 16);
	}
	return result;
}

UInt8  EvtEventAvail (void){
	UInt8 result;
	{
		STACK_SIZE( 0);
		result=(UInt8)STACK_RUN(sysTrapEvtEventAvail,0);
	}
	return result;
}

Err MemPtrResize (MemPtr p, UInt32 newSize){
	Err result;
     
	{
		STACK_SIZE( 8);
		STACK_ADD32( p, 0);
		STACK_ADD32( newSize, 4);
		result = (Err)STACK_RUN((sysTrapMemPtrResize),8 );
	}
	return result;
}

WinDrawOperation WinSetDrawMode (WinDrawOperation newMode){
	WinDrawOperation result;
	{
		STACK_SIZE( 2);
		STACK_ADD8( newMode, 0);
		result = (WinDrawOperation)STACK_RUN((sysTrapWinSetDrawMode),2 );
	}
	return result;
}

void WinSetForeColorRGB(const RGBColorType *newRgbP, RGBColorType *prevRgbP)
{
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( newRgbP, 0);
		STACK_ADD32( prevRgbP, 4);
  
		STACK_RUN( (sysTrapWinSetForeColorRGB),8 );
 
	}
}

void *SysGetTrapAddress (UInt16 trapNum){
	void * result;
	{
		STACK_SIZE( 2);
		STACK_ADD16( trapNum, 0);
		result = (void *)STACK_RUN_RET_PTR( (sysTrapSysGetTrapAddress),2 );
	}
	return result;
}

void WinSetBackColorRGB(const RGBColorType *newRgbP, RGBColorType *prevRgbP)
{
	{

		STACK_SIZE( 8);
     
		STACK_ADD32( newRgbP, 0);
		STACK_ADD32( prevRgbP, 4);
  
		STACK_RUN( (sysTrapWinSetBackColorRGB),8 );
 
	}
}


WinHandle WinGetDisplayWindow (){
	WinHandle result;
	{
		STACK_SIZE( 0);
		result=(WinHandle)STACK_RUN_RET_PTR( (sysTrapWinGetDisplayWindow),0 );
	}
	return result;
}

FormType *FrmNewForm (UInt16 formID, const Char *titleStrP,
	Coord x, Coord y, Coord width, Coord height, Boolean modal,
	UInt16 defaultButton, UInt16 helpRscID, UInt16 menuRscID)
{
	FormType *result;

	{
		STACK_SIZE(22);
		STACK_ADD16(formID,0);
		STACK_ADD32(titleStrP,2);
		STACK_ADD16(x,6);
		STACK_ADD16(y,8);
		STACK_ADD16(width,10);
		STACK_ADD16(height,12);
		STACK_ADD8(modal,14);
		STACK_ADD16(defaultButton,16);
		STACK_ADD16(helpRscID,18);
		STACK_ADD16(menuRscID,20);

		result=(FormType *)STACK_RUN_RET_PTR((sysTrapFrmNewForm),22);
	}

	return result;
}

WinHandle FrmGetWindowHandle (const FormType *formP)
{
	WinHandle result;

	{
		STACK_SIZE(4);
		STACK_ADD32(formP,0);
		result=(WinHandle)STACK_RUN_RET_PTR((sysTrapFrmGetWindowHandle),4);
	}

	return result;
}

Err		WinSetConstraintsSize(WinHandle winH, Coord minH, Coord prefH, Coord maxH,
							Coord minW, Coord prefW, Coord maxW)
{
	Err result;

	{
		STACK_SIZE(16);
		STACK_ADD32(winH,0);
		STACK_ADD16(minH,4);
		STACK_ADD16(prefH,6);
		STACK_ADD16(maxH,8);
		STACK_ADD16(minW,10);
		STACK_ADD16(prefW,12);
		STACK_ADD16(maxW,14);

		SET_SEL_REG(pinWinSetConstraintsSize, sp);
		result = (Err)STACK_RUN((sysTrapPinsDispatch),16 );
	}

	return result;
}

Err		FrmSetDIAPolicyAttr (FormPtr formP, UInt16 diaPolicy)
{
	Err result;

	{
		STACK_SIZE(6);
		STACK_ADD32(formP,0);
		STACK_ADD16(diaPolicy,4);

		SET_SEL_REG(pinFrmSetDIAPolicyAttr, sp);
		result = (Err)STACK_RUN((sysTrapPinsDispatch),6 );
	}

	return result;
}

void FrmSetActiveForm (FormType *formP)
{
	{
		STACK_SIZE(4);
		STACK_ADD32(formP,0);

		STACK_RUN((sysTrapFrmSetActiveForm),4);
	}
}

void FrmDrawForm (FormType *formP)
{
	{
		STACK_SIZE(4);
		STACK_ADD32(formP,0);

		STACK_RUN((sysTrapFrmDrawForm),4);
	}
}

SysAppInfoPtr SysGetAppInfo(SysAppInfoPtr *uiAppPP, SysAppInfoPtr *actionCodeAppPP)
{
	SysAppInfoPtr result;

	{
		STACK_SIZE(8);
		STACK_ADD32(uiAppPP,0);
		STACK_ADD32(actionCodeAppPP,4);

		result=(SysAppInfoPtr)STACK_RUN_RET_PTR((sysTrapSysGetAppInfo),8);
	}

	return result;
}

Err SndStreamDelete(SndStreamRef channel)
{
	Err result;

	{
		STACK_SIZE(4);
		STACK_ADD32(channel,0);

		result=(Err)STACK_RUN((sysTrapSndStreamDelete),4);
	}

	return result;
}

Err SndStreamStart(SndStreamRef channel)
{
	Err result;

	{
		STACK_SIZE(4);
		STACK_ADD32(channel,0);

		result=(Err)STACK_RUN((sysTrapSndStreamStart),4);
	}

	return result;
}

Err SndStreamPause(SndStreamRef channel, Boolean pause)
{
	Err result;

	{
		STACK_SIZE(6);
		STACK_ADD32(channel,0);
		STACK_ADD8(pause,4);

		result=(Err)STACK_RUN((sysTrapSndStreamPause),6);
	}

	return result;
}

Err SndStreamStop(SndStreamRef channel)
{
	Err result;

	{
		STACK_SIZE(4);
		STACK_ADD32(channel,0);

		result=(Err)STACK_RUN((sysTrapSndStreamStop),4);
	}

	return result;
}

Err SndStreamCreate(SndStreamRef *channel,	
						SndStreamMode mode,			
						UInt32 samplerate,				
						SndSampleType type,				
						SndStreamWidth width,			
						SndStreamBufferCallback func,	
						void *userdata,					
						UInt32 buffsize,				
						Boolean armNative)				
{
	Err result;

	{
		STACK_SIZE(28);
		STACK_ADD32(channel,0);
		STACK_ADD8(mode,4);
		STACK_ADD32(samplerate,6);
		STACK_ADD16(type,10);
		STACK_ADD8(width,12);
		STACK_ADD32(func,14);
		STACK_ADD32(userdata,18);
		STACK_ADD32(buffsize,22);
		STACK_ADD8(armNative,26);

		result=(Err)STACK_RUN((sysTrapSndStreamCreate),28);
	}	

	SWAP4_NON_NULL(channel);

	return result;
}

Err SndStreamSetVolume(SndStreamRef channel, Int32 volume)
{
	Err result;

	{
		STACK_SIZE(8);
		STACK_ADD32(channel,0);
		STACK_ADD32(volume,4);

		result=(Err)STACK_RUN((sysTrapSndStreamSetVolume),8);
	}

	return result;
}

Err SilkLibOpen(UInt16 refNum)
{
	Err result;

	STACK_SIZE(2);
	STACK_ADD16(refNum,0);
	result=(Err)STACK_RUN(sysLibTrapOpen,2);

	return result;
}

Err SilkLibClose(UInt16 refNum)
{
	Err result;

	STACK_SIZE(2);
	STACK_ADD16(refNum,0);
	result=(Err)STACK_RUN(sysLibTrapClose,2);

	return result;
}

Boolean HwrEnableDataWrites()
{
	Boolean result;

	{
		STACK_SIZE(0);
		result=(Boolean)STACK_RUN(sysTrapHwrEnableDataWrites,0);
	}

	return result;
}

void HwrDisableDataWrites()
{
	STACK_SIZE(0);
	STACK_RUN(sysTrapHwrDisableDataWrites,0);
}


UInt16	SysBatteryInfo(Boolean set, UInt16 *warnThresholdP, UInt16 *criticalThresholdP,
						Int16 *maxTicksP, SysBatteryKind* kindP, Boolean *pluggedIn, UInt8 *percentP)
{
	UInt16 result;

	SWAP2_NON_NULL(warnThresholdP);
	SWAP2_NON_NULL(criticalThresholdP);
	SWAP2_NON_NULL(maxTicksP);
	//SysBatteryKind!!!

	STACK_SIZE(26);
	STACK_ADD8(set,0);
	STACK_ADD32(warnThresholdP,2);
	STACK_ADD32(criticalThresholdP,6);
	STACK_ADD32(maxTicksP,10);
	STACK_ADD32(kindP,14);
	STACK_ADD32(pluggedIn,18);
	STACK_ADD32(percentP,22);

	result=(UInt16)STACK_RUN(sysTrapSysBatteryInfo,26);

	SWAP2_NON_NULL(warnThresholdP);
	SWAP2_NON_NULL(criticalThresholdP);
	SWAP2_NON_NULL(maxTicksP);

	return result;
}


#endif
