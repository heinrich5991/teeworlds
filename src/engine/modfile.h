/* (c) 2011 MAP94 and Patafix*/
#ifndef ENGINE_MODFILE_H
#define ENGINE_MODFILE_H

class CModFile
{
    public:
	    char m_aName[256];
	    char m_aFileDir[1024];
	    enum FILETYPE
	    {
	        FILETYPEINVALID = 0,
	        FILETYPELUA,
	        FILETYPEPNG,
	        FILETYPEWAV,
	        FILETYPEWV,
	        FILETYPEOTHER,
	    } m_Type;
		enum FILEFLAGS
	    {
	        FILEFLAG_IGNORETYPE = 1,
	        FILEFLAG_NOCRC = 2,
	        FILEFLAG_LAUNCH = 4,
	        FILEFLAG_SAVE = 8,
			FILEFLAG_CHECK = 16,
			FILEFLAG_UPDATE = 32,
	    };
		int m_Flags;
        int m_Size;
        int m_Crc;
        unsigned char *m_pCurrentData;
};
#endif
