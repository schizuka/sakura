/*
	Copyright (C) 2002, SUI
	Copyright (C) 2003, MIK
	Copyright (C) 2008, kobake

	This software is provided 'as-is', without any express or implied
	warranty. In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose, 
	including commercial applications, and to alter it and redistribute it 
	freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented;
		   you must not claim that you wrote the original software.
		   If you use this software in a product, an acknowledgment
		   in the product documentation would be appreciated but is
		   not required.

		2. Altered source versions must be plainly marked as such, 
		   and must not be misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		   distribution.
*/

#include "StdAfx.h"
#include <io.h>
#include "etc_uty.h"
#include "file.h"
#include "CShareData.h"
#include "charcode.h"
#include "my_icmp.h"

//	Sep. 10, 2005 genta GetLongPathName�̃G�~�����[�V�����֐��̎��̐����̂���
#define COMPILE_NEWAPIS_STUBS

bool fexist(LPCTSTR pszPath)
{
	return _taccess(pszPath,0)!=-1;
}

/**	�t�@�C�����̐؂�o��

	�w�蕶���񂩂�t�@�C�����ƔF������镶��������o���A
	�擪Offset�y�ђ�����Ԃ��B
	
	@retval true �t�@�C��������
	@retval false �t�@�C�����͌�����Ȃ�����
	
	@date 2002.01.04 genta �t�@�C�����݊m�F���@�ύX
	@date 2002.01.04 genta �f�B���N�g���������ΏۊO�ɂ���@�\��ǉ�
	@date 2003.01.15 matsumo gcc�̃G���[���b�Z�[�W(:��؂�)�ł��t�@�C�������o�\��
	@date 2004.05.29 genta C:\����t�@�C��C���؂�o�����̂�h�~
	@date 2004.11.13 genta/Moca �t�@�C�����擪��*?���l��
	@date 2005.01.10 genta �ϐ����ύX j -> cur_pos
	@date 2005.01.23 genta �x���}���̂��߁Cgoto��return�ɕύX
*/
bool IsFilePath(
	const char* pLine,		//!< [in] �T���Ώە�����
	int* pnBgn,				//!< [out] �擪offset�BpLine + *pnBgn���t�@�C�����擪�ւ̃|�C���^�B
	int* pnPathLen,			//!< [out] �t�@�C�����̒���
	bool bFileOnly			//!< [in] true: �t�@�C���̂ݑΏ� / false: �f�B���N�g�����Ώ�
)
{
	char	szJumpToFile[1024];
	memset( szJumpToFile, 0, _countof( szJumpToFile ) );

	int nLineLen = strlen( pLine );

	//�擪�̋󔒂�ǂݔ�΂�
	int		i;
	for( i = 0; i < nLineLen; ++i ){
		char c = pLine[i];
		if( ' ' != c && '\t' != c && '\"' != c ){
			break;
		}
	}

	//	#include <�t�@�C����>�̍l��
	//	#�Ŏn�܂�Ƃ���"�܂���<�܂œǂݔ�΂�
	if( i < nLineLen && '#' == pLine[i] ){
		for( ; i < nLineLen; ++i ){
			if( '<'  == pLine[i] || '\"' == pLine[i] ){
				++i;
				break;
			}
		}
	}

	//	���̎��_�Ŋ��ɍs���ɒB���Ă�����t�@�C�����͌�����Ȃ�
	if( i >= nLineLen ){
		return false;
	}

	*pnBgn = i;
	int cur_pos = 0;
	for( ; i <= nLineLen && cur_pos + 1 < _countof(szJumpToFile); ++i ){
		if( ( i == nLineLen    ||
			  pLine[i] == ' '  ||
			  pLine[i] == '\t' ||	//@@@ 2002.01.08 YAZAKI �^�u�������B
			  pLine[i] == '('  ||
			  pLine[i] == '\r' ||
			  pLine[i] == '\n' ||
			  pLine[i] == '\0' ||
			  pLine[i] == '>'  ||
			  // May 29, 2004 genta C:\��:�̓t�@�C����؂�ƌ��Ȃ��ė~�����Ȃ�
			  ( cur_pos > 1 && pLine[i] == ':' ) ||   //@@@ 2003/1/15/ matsumo (for gcc)
			  pLine[i] == '"'
			) &&
			szJumpToFile[0] != '\0' 
		){
			if( IsFileExists(szJumpToFile, bFileOnly))
			{
				i--;
				break;
			}
		}
		if( pLine[i] == '\r'  ||
			pLine[i] == '\n' ){
			break;
		}

		if( ( /*pLine[i] == '/' ||*/
			 pLine[i] == '<' ||	//	0x3C
			 pLine[i] == '>' ||	//	0x3E
			 pLine[i] == '?' ||	//	0x3F
			 pLine[i] == '"' ||	//	0x22
			 pLine[i] == '|' ||	//	0x7C
			 pLine[i] == '*'	//	0x2A
			) &&
			/* ��̕�����SJIS2�o�C�g�R�[�h��2�o�C�g�ڂłȂ����Ƃ��A1�O�̕�����SJIS2�o�C�g�R�[�h��1�o�C�g�ڂłȂ����ƂŔ��f���� */
			//	Oct. 5, 2002 genta
			//	2004.11.13 Moca/genta �擪�ɏ�̕���������ꍇ�̍l����ǉ�
			( i == 0 || ( i > 0 && ! _IS_SJIS_1( (unsigned char)pLine[i - 1] ))) ){
			return false;
		}else{
		szJumpToFile[cur_pos] = pLine[i];
		cur_pos++;
		}
	}

	if( szJumpToFile[0] != _T('\0')  && IsFileExists(szJumpToFile, bFileOnly)){
		*pnPathLen = strlen( szJumpToFile );
		return true;
	}

	return false;
}

/*! fname�����΃p�X�̏ꍇ�́A���s�t�@�C���̃p�X����̑��΃p�X�Ƃ��ĊJ��
	@author Moca
	@date 2003.06.23
	@date 2007.05.20 ryoji �֐����ύX�i���Ffopen_absexe�j�A�ėp�e�L�X�g�}�b�s���O��
*/
FILE* _tfopen_absexe(LPCTSTR fname, LPCTSTR mode)
{
	if( _IS_REL_PATH( fname ) ){
		TCHAR path[_MAX_PATH];
		GetExedir( path, fname );
		return _tfopen( path, mode );
	}
	return _tfopen( fname, mode );
}

/*! fname�����΃p�X�̏ꍇ�́AINI�t�@�C���̃p�X����̑��΃p�X�Ƃ��ĊJ��
	@author ryoji
	@date 2007.05.19 �V�K�쐬�i_tfopen_absexe�x�[�X�j
*/
FILE* _tfopen_absini(LPCTSTR fname, LPCTSTR mode, BOOL bOrExedir/*=TRUE*/ )
{
	if( _IS_REL_PATH( fname ) ){
		TCHAR path[_MAX_PATH];
		if( bOrExedir )
			GetInidirOrExedir( path, fname );
		else
			GetInidir( path, fname );
		return _tfopen( path, mode );
	}
	return _tfopen( fname, mode );
}



/* �t�H���_�̍Ōオ���p����'\\'�̏ꍇ�́A��菜�� "c:\\"���̃��[�g�͎�菜���Ȃ� */
void CutLastYenFromDirectoryPath( TCHAR* pszFolder )
{
	if( 3 == _tcslen( pszFolder )
	 && pszFolder[1] == _T(':')
	 && pszFolder[2] == _T('\\')
	){
		/* �h���C�u��:\ */
	}else{
		/* �t�H���_�̍Ōオ���p����'\\'�̏ꍇ�́A��菜�� */
		int	nFolderLen;
		int	nCharChars;
		nFolderLen = _tcslen( pszFolder );
		if( 0 < nFolderLen ){
			nCharChars = &pszFolder[nFolderLen] - CMemory::MemCharPrev( pszFolder, nFolderLen, &pszFolder[nFolderLen] );
			if( 1 == nCharChars && _T('\\') == pszFolder[nFolderLen - 1] ){
				pszFolder[nFolderLen - 1] = _T('\0');
			}
		}
	}
	return;
}




/* �t�H���_�̍Ōオ���p����'\\'�łȂ��ꍇ�́A�t������ */
void AddLastYenFromDirectoryPath( TCHAR* pszFolder )
{
	if( 3 == _tcslen( pszFolder )
	 && pszFolder[1] == _T(':')
	 && pszFolder[2] == _T('\\')
	){
		/* �h���C�u��:\ */
	}else{
		/* �t�H���_�̍Ōオ���p����'\\'�łȂ��ꍇ�́A�t������ */
		int	nFolderLen;
		int	nCharChars;
		nFolderLen = _tcslen( pszFolder );
		if( 0 < nFolderLen ){
			nCharChars = &pszFolder[nFolderLen] - CMemory::MemCharPrev( pszFolder, nFolderLen, &pszFolder[nFolderLen] );
			if( 1 == nCharChars && _T('\\') == pszFolder[nFolderLen - 1] ){
			}else{
				pszFolder[nFolderLen] = _T('\\');
				pszFolder[nFolderLen + 1] = _T('\0');
			}
		}
	}
	return;
}



/* �t�@�C���̃t���p�X���A�t�H���_�ƃt�@�C�����ɕ��� */
/* [c:\work\test\aaa.txt] �� [c:\work\test] + [aaa.txt] */
void SplitPath_FolderAndFile( const TCHAR* pszFilePath, TCHAR* pszFolder, TCHAR* pszFile )
{
	TCHAR	szDrive[_MAX_DRIVE];
	TCHAR	szDir[_MAX_DIR];
	TCHAR	szFname[_MAX_FNAME];
	TCHAR	szExt[_MAX_EXT];
	int		nFolderLen;
	int		nCharChars;
	_tsplitpath( pszFilePath, szDrive, szDir, szFname, szExt );
	if( NULL != pszFolder ){
		_tcscpy( pszFolder, szDrive );
		_tcscat( pszFolder, szDir );
		/* �t�H���_�̍Ōオ���p����'\\'�̏ꍇ�́A��菜�� */
		nFolderLen = _tcslen( pszFolder );
		if( 0 < nFolderLen ){
			nCharChars = &pszFolder[nFolderLen] - CMemory::MemCharPrev( pszFolder, nFolderLen, &pszFolder[nFolderLen] );
			if( 1 == nCharChars && _T('\\') == pszFolder[nFolderLen - 1] ){
				pszFolder[nFolderLen - 1] = _T('\0');
			}
		}
	}
	if( NULL != pszFile ){
		_tcscpy( pszFile, szFname );
		_tcscat( pszFile, szExt );
	}
	return;
}


/*! �����O�t�@�C�������擾���� 

	@param[in] pszFilePathSrc �ϊ����p�X��
	@param[out] pszFilePathDes ���ʏ������ݐ� (����MAX_PATH�̗̈悪�K�v)

	@date Oct. 2, 2005 genta GetFilePath API���g���ď�������
	@date Oct. 4, 2005 genta ���΃p�X����΃p�X�ɒ�����Ȃ�����
	@date Oct. 5, 2005 Moca  ���΃p�X���΃p�X�ɕϊ�����悤��
*/
BOOL GetLongFileName( const TCHAR* pszFilePathSrc, TCHAR* pszFilePathDes )
{
	TCHAR* name;
	TCHAR szBuf[_MAX_PATH + 1];
	int len = ::GetFullPathName( pszFilePathSrc, _MAX_PATH, szBuf, &name );
	if( len <= 0 || _MAX_PATH <= len ){
		len = ::GetLongPathName( pszFilePathSrc, pszFilePathDes, _MAX_PATH );
		if( len <= 0 || _MAX_PATH < len ){
			return FALSE;
		}
		return TRUE;
	}
	len = ::GetLongPathName( szBuf, pszFilePathDes, _MAX_PATH );
	if( len <= 0 || _MAX_PATH < len ){
		_tcscpy( pszFilePathDes, szBuf );
	}
	return TRUE;
}


/* �g���q�𒲂ׂ� */
BOOL CheckEXT( const TCHAR* pszPath, const TCHAR* pszExt )
{
	TCHAR	szExt[_MAX_EXT];
	TCHAR*	pszWork;
	_tsplitpath( pszPath, NULL, NULL, NULL, szExt );
	pszWork = szExt;
	if( pszWork[0] == _T('.') ){
		pszWork++;
	}
	if( 0 == my_stricmp( pszExt, pszWork ) ){
		return TRUE;
	}else{
		return FALSE;
	}
}

/*! ���΃p�X�����肷��
	@author Moca
	@date 2003.06.23
*/
bool _IS_REL_PATH(const TCHAR* path)
{
	bool ret = true;
	if( ( _T('A') <= path[0] && path[0] <= _T('Z') || _T('a') <= path[0] && path[0] <= _T('z') )
		&& path[1] == _T(':') && path[2] == _T('\\')
		|| path[0] == _T('\\') && path[1] == _T('\\')
		 ){
		ret = false;
	}
	return ret;
}

/*!
	@brief exe�t�@�C���̂���f�B���N�g���C�܂��͎w�肳�ꂽ�t�@�C�����̃t���p�X��Ԃ��D
	
	@author genta
	@date 2002.12.02 genta
	@date 2007.05.20 ryoji �֐����ύX�i���FGetExecutableDir�j�A�ėp�e�L�X�g�}�b�s���O��
	@date 2008.05.05 novice GetModuleHandle(NULL)��NULL�ɕύX
*/
void GetExedir(
	LPTSTR pDir,		//!< [out] EXE�t�@�C���̂���f�B���N�g����Ԃ��ꏊ�D�\��_MAX_PATH�̃o�b�t�@��p�ӂ��Ă������ƁD
	LPCTSTR szFile		//!< [in] �f�B���N�g�����Ɍ�������t�@�C�����D  
)
{
	if( pDir == NULL )
		return;
	
	TCHAR	szPath[_MAX_PATH];
	// sakura.exe �̃p�X���擾
	::GetModuleFileName( NULL, szPath, _countof(szPath) );
	if( szFile == NULL ){
		SplitPath_FolderAndFile( szPath, pDir, NULL );
	}
	else {
		TCHAR	szDir[_MAX_PATH];
		SplitPath_FolderAndFile( szPath, szDir, NULL );
		_snprintf( pDir, _MAX_PATH, _T("%s\\%s"), szDir, szFile );
		pDir[_MAX_PATH - 1] = _T('\0');
	}
}

/*!
	@brief INI�t�@�C���̂���f�B���N�g���C�܂��͎w�肳�ꂽ�t�@�C�����̃t���p�X��Ԃ��D
	
	@author ryoji
	@date 2007.05.19 �V�K�쐬�iGetExedir�x�[�X�j
*/
void GetInidir(
	LPTSTR pDir,				//!< [out] INI�t�@�C���̂���f�B���N�g����Ԃ��ꏊ�D�\��_MAX_PATH�̃o�b�t�@��p�ӂ��Ă������ƁD
	LPCTSTR szFile	/*=NULL*/	//!< [in] �f�B���N�g�����Ɍ�������t�@�C�����D
)
{
	if( pDir == NULL )
		return;
	
	TCHAR	szPath[_MAX_PATH];
	// sakura.ini �̃p�X���擾
	CShareData::getInstance()->GetIniFileName( szPath );
	if( szFile == NULL ){
		SplitPath_FolderAndFile( szPath, pDir, NULL );
	}
	else {
		TCHAR	szDir[_MAX_PATH];
		SplitPath_FolderAndFile( szPath, szDir, NULL );
		_snprintf( pDir, _MAX_PATH, _T("%s\\%s"), szDir, szFile );
		pDir[_MAX_PATH - 1] = _T('\0');
	}
}


/*!
	@brief INI�t�@�C���܂���EXE�t�@�C���̂���f�B���N�g���C�܂��͎w�肳�ꂽ�t�@�C�����̃t���p�X��Ԃ��iINI��D��j�D
	
	@author ryoji
	@date 2007.05.22 �V�K�쐬
*/
void GetInidirOrExedir(
	LPTSTR pDir,							//!< [out] INI�t�@�C���܂���EXE�t�@�C���̂���f�B���N�g����Ԃ��ꏊ�D
											//         �\��_MAX_PATH�̃o�b�t�@��p�ӂ��Ă������ƁD
	LPCTSTR szFile				/*=NULL*/,	//!< [in] �f�B���N�g�����Ɍ�������t�@�C�����D
	BOOL bRetExedirIfFileEmpty	/*=FALSE*/	//!< [in] �t�@�C�����̎w�肪��̏ꍇ��EXE�t�@�C���̃t���p�X��Ԃ��D	
)
{
	TCHAR	szInidir[_MAX_PATH];
	TCHAR	szExedir[_MAX_PATH];

	// �t�@�C�����̎w�肪��̏ꍇ��EXE�t�@�C���̃t���p�X��Ԃ��i�I�v�V�����j
	if( bRetExedirIfFileEmpty && (szFile == NULL || szFile[0] == _T('\0')) ){
		GetExedir( szExedir, szFile );
		::lstrcpy( pDir, szExedir );
		return;
	}

	// INI��̃t���p�X�����݂���΂��̃p�X��Ԃ�
	GetInidir( szInidir, szFile );
	if( fexist(szInidir) ){
		::lstrcpy( pDir, szInidir );
		return;
	}

	// EXE��̃t���p�X�����݂���΂��̃p�X��Ԃ�
	if( CShareData::getInstance()->IsPrivateSettings() ){	// INI��EXE�Ńp�X���قȂ�ꍇ
		GetExedir( szExedir, szFile );
		if( fexist(szExedir) ){
			::lstrcpy( pDir, szExedir );
			return;
		}
	}

	// �ǂ���ɂ����݂��Ȃ����INI��̃t���p�X��Ԃ�
	::lstrcpy( pDir, szInidir );
}

/**	�t�@�C���̑��݃`�F�b�N

	�w�肳�ꂽ�p�X�̃t�@�C�������݂��邩�ǂ������m�F����B
	
	@param path [in] ���ׂ�p�X��
	@param bFileOnly [in] true: �t�@�C���̂ݑΏ� / false: �f�B���N�g�����Ώ�
	
	@retval true  �t�@�C���͑��݂���
	@retval false �t�@�C���͑��݂��Ȃ�
	
	@author genta
	@date 2002.01.04 �V�K�쐬
*/
bool IsFileExists(const TCHAR* path, bool bFileOnly)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile( path, &fd );
	if( hFind != INVALID_HANDLE_VALUE ){
		::FindClose( hFind );
		if( bFileOnly && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )return false;
			return true;
	}
	return false;
}

/*!	�t�@�C���̍X�V�������擾

	@return true: ����, false: FindFirstFile���s

	@author genta by assitance with ryoji
	@date 2005.10.22 new

	@note �������݌�Ƀt�@�C�����ăI�[�v�����ă^�C���X�^���v�𓾂悤�Ƃ����
	�t�@�C�����܂����b�N����Ă��邱�Ƃ�����C�㏑���֎~�ƌ�F����邱�Ƃ�����D
	FindFirstFile���g�����ƂŃt�@�C���̃��b�N��Ԃɉe�����ꂸ�Ƀ^�C���X�^���v��
	�擾�ł���D(ryoji)
*/
bool GetLastWriteTimestamp(
	const TCHAR*	pszFileName,	//!< [in] �t�@�C���̃p�X
	CFileTime*		pcFileTime		//!< [out] �X�V������Ԃ��ꏊ
)
{
	HANDLE hFindFile;
	WIN32_FIND_DATA ffd;

	hFindFile = ::FindFirstFile( pszFileName, &ffd );
	if( INVALID_HANDLE_VALUE != hFindFile )
	{
		::FindClose( hFindFile );
		pcFileTime->SetFILETIME(ffd.ftLastWriteTime);
		return true;
	}
	else{
		//	�t�@�C����������Ȃ�����
		pcFileTime->ClearFILETIME();
		return false;
	}
}








// -----------------------------------------------------------------------------
//
//
//                   MY_SP.c by SUI
//
//



/* ============================================================================
my_splitpath( const char *CommandLine, char *drive, char *dir, char *fname, char *ext );

�� �T�v
CommandLine �ɗ^����ꂽ�R�}���h���C��������̐擪����A���݂���t�@�C���E�f�B
���N�g���̕����݂̂𒊏o���A���̒��o�����ɑ΂��� _splitpath() �Ɠ����̏�������
���Ȃ��܂��B
�擪�����Ɏ��݂���t�@�C���E�f�B���N�g�����������ꍇ�͋󕶎��񂪕Ԃ�܂��B
�����񒆂̓��{��(Shift_JIS�R�[�h�̂�)�ɑΉ����Ă��܂��B

�� �v���O�����L�q���e�ɂ���(�����󂠂ꂱ��)
������� split �����������ɂł� _splitpath() ���g���΂����ƒZ���Ȃ����̂ł����A
������炸�ɑS�Ď��O�ŏ������Ă��闝�R�́A
�E�R���p�C���ɂ���Ă� _splitpath() �����{��ɑΉ����Ă��Ȃ��\��������B
�E_splitpath() ���āA�R���p�C���ɂ���āA�ڍד��삪�����ɈقȂ邩������Ȃ�����
�@�d�l���n�b�L�������邽�߂ɂ��R���p�C���ɓY�t����Ă��� _splitpath() �ɂ��܂�
�@���肽���Ȃ������B
�E�Ƃ������A��ɓ���m�F�Ɏg�p���Ă��� LSI-C���H�łɂ͂������� _splitpath() ��
�@���݂��Ȃ�����A��炴������Ȃ������B :-(
�Ƃ������ɂ��܂��B

�� "LFN library" -> http://webs.to/ken/

�� �ڍד���
my_splitpath( CommandLine, drive, dir, fname, ext );
CommandLine �ɕ�����Ƃ��� D:\Test.ext ���^����ꂽ�ꍇ�A
���ED:\Test.ext �Ƃ����t�@�C�������݂���ꍇ
���@drive = "D:"
���@dir   = "\"
���@fname = "Test"
���@ext   = ".ext"
���ED:\Test.ext �Ƃ����f�B���N�g�������݂���ꍇ
���@drive = "D:"
���@dir   = "\Test.ext\"
���@fname = ""
���@ext   = ""
���ED:\Test.ext �Ƃ����t�@�C���E�f�B���N�g�������݂��Ȃ��ꍇ�A
�@�@���ED:�h���C�u�͗L��
�@�@���@drive = "D:"
�@�@���@dir   = "\"
�@�@���@fname = ""
�@�@���@ext   = ""
�@�@���ED:�h���C�u�͖���
�@�@�@�@drive = ""
�@�@�@�@dir   = ""
�@�@�@�@fname = ""
�@�@�@�@ext   = ""
)=========================================================================== */

/* Shift_JIS �Ή��Ō����Ώە������Q�w��ł��� strrchr() �݂����Ȃ��́B
/ �w�肳�ꂽ�Q�̕����̂����A����������(������̕�)�̈ʒu��Ԃ��B
/ # strrchr( char *s , char c ) �Ƃ́A������ s ���̍Ō���� c ��T���o���֐��B
/ # ���� c ������������A���̈ʒu��Ԃ��B
/ # ���� c ��������Ȃ��ꍇ�� NULL ��Ԃ��B */
char *sjis_strrchr2( const char *pt , const char ch1 , const char ch2 ){
	const char *pf = NULL;
	while( *pt != '\0' ){	/* ������̏I�[�܂Œ��ׂ�B */
		if( ( *pt == ch1 ) || ( *pt == ch2 ) )	pf = pt;	/* pf = ���������̈ʒu */
		if( _IS_SJIS_1(*pt) )	pt++;	/* Shift_JIS ��1�����ڂȂ�A����1�������X�L�b�v */
		if( *pt != '\0' )		pt++;	/* ���̕����� */
	}
	return	(char *)pf;
}

#define		GetExistPath_NO_DriveLetter	0	/* �h���C�u���^�[������ */
#define		GetExistPath_IV_Drive		1	/* �h���C�u������ */
#define		GetExistPath_AV_Drive		2	/* �h���C�u���L�� */

void	GetExistPath( char *po , const char *pi )
{
	char	*pw,*ps;
	int		cnt;
	char	drv[4] = "_:\\";
	int		dl;		/* �h���C�u�̏�� */

	/* pi �̓��e��
	/ �E " ���폜����
	/ �E / �� \ �ɕϊ�����(Win32API �ł� / �� \ �Ɠ����Ɉ����邩��)
	/ �E�ő� ( _MAX_PATH -1 ) �����܂�
	/ po �ɃR�s�[����B */
	for( pw=po,cnt=0 ; ( *pi != '\0' ) && ( cnt < _MAX_PATH -1 ) ; pi++ ){
		/* /," ���� Shift_JIS �̊����R�[�h���ɂ͊܂܂�Ȃ��̂� Shift_JIS ����͕s�v�B */
		if( *pi == '\"' )	continue;		/*  " �Ȃ牽�����Ȃ��B���̕����� */
		if( *pi == '/' )	*pw++ = '\\';	/*  / �Ȃ� \ �ɕϊ����ăR�s�[    */
		else				*pw++ = *pi;	/* ���̑��̕����͂��̂܂܃R�s�[  */
		cnt++;	/* �R�s�[���������� ++ */
	}
	*pw = '\0';		/* ������I�[ */

	dl = GetExistPath_NO_DriveLetter;	/*�u�h���C�u���^�[�������v�ɂ��Ă���*/
	if(
		( *(po+1) == ':' )&&
		(
			( ( *po >='A' )&&( *po <='Z' ) ) ||
			( ( *po >='a' )&&( *po <='z' ) )
		)
	){	/* �擪�Ƀh���C�u���^�[������B���̃h���C�u���L�����ǂ������肷�� */
		drv[0] = *po;
		if( access(drv,0) == 0 )	dl = GetExistPath_AV_Drive;		/* �L�� */
		else						dl = GetExistPath_IV_Drive;		/* ���� */
	}

	if( dl == GetExistPath_IV_Drive ){	/* �h���C�u���̂����� */
		/* �t���b�s�[�f�B�X�N���̃t�@�C�����w�肳��Ă��āA
		�@ ���̃h���C�u�Ƀt���b�s�[�f�B�X�N�������Ă��Ȃ��A�Ƃ� */
		*po = '\0';	/* �Ԓl������ = "";(�󕶎���) */
		return;		/* ����ȏ㉽�����Ȃ� */
	}

	/* ps = �����J�n�ʒu */
	ps = po;	/* ��������̐擪�� \\ �Ȃ�A\ ���������̑Ώۂ���O�� */
	if( ( *po == '\\' )&&( *(po+1) == '\\' ) )	ps +=2;

	if( *ps == '\0' ){	/* �����Ώۂ��󕶎���Ȃ� */
		*po = '\0';		/* �Ԓl������ = "";(�󕶎���) */
		return;			/*����ȏ㉽�����Ȃ� */
	}

	for(;;){
		if( access(po,0) == 0 )	break;	/* �L���ȃp�X�����񂪌������� */
		/* ��������Ō���� \ �܂��� ' ' ��T���o���A�����𕶎���I�[�ɂ���B*/

		pw = sjis_strrchr2(ps,'\\',' ');	/* �Ŗ����� \ �� ' ' ��T���B */
		if ( pw == NULL ){	/* �����񒆂� '\\' �� ' ' ���������� */
			/* �Ⴆ�� "C:testdir" �Ƃ��������񂪗������ɁA"C:testdir" ������
			�@ ���Ȃ��Ƃ� C:�h���C�u���L���Ȃ� "C:" �Ƃ��������񂾂��ł��Ԃ�
			�@ �����B�ȉ����́A���̂��߂̏����B */
			if( dl == GetExistPath_AV_Drive ){
				/* �擪�ɗL���ȃh���C�u�̃h���C�u���^�[������B */
				*(po+2) = '\0';		/* �h���C�u���^�[���̕�����̂ݕԂ� */
			}
			else{	/* �L���ȃp�X�������S��������Ȃ����� */
				*po = '\0';	/* �Ԓl������ = "";(�󕶎���) */
			}
			break;		/* ���[�v�𔲂��� */
		}
		/* �����[�g�f�B���N�g�������������邽�߂̏��� */
		if( ( *pw == '\\' )&&( *(pw-1) == ':' ) ){	/* C:\ �Ƃ��� \ ���ۂ� */
			* (pw+1) = '\0';		/* \ �̌��̈ʒu�𕶎���̏I�[�ɂ���B */
			if( access(po,0) == 0 )	break;	/* �L���ȃp�X�����񂪌������� */
		}
		*pw = '\0';		/* \ �� ' ' �̈ʒu�𕶎���̏I�[�ɂ���B */
		/* ���������X�y�[�X�Ȃ�A�X�y�[�X��S�č폜���� */
		while( ( pw != ps ) && ( *(pw-1) == ' ' ) )	* --pw = '\0';
	}

	return;
}

/* �^����ꂽ�R�}���h���C��������̐擪����������݂���t�@�C���E�f�B���N�g��
�@ �̃p�X������𒊏o���A���̃p�X�𕪉����� drv dir fnm ext �ɏ������ށB
�@ �擪�����ɗL���ȃp�X�������݂��Ȃ��ꍇ�A�S�Ăɋ󕶎��񂪕Ԃ�B */
void	my_splitpath ( const char *comln , char *drv,char *dir,char *fnm,char *ext )
{
	char	ppp[_MAX_PATH];		/* �p�X�i�[�i��Ɨp�j */
	char	*pd;
	char	*pf;
	char	*pe;
	char	ch;
	DWORD	attr;
	int		a_dir;

	if( drv != NULL )	*drv = '\0';
	if( dir != NULL )	*dir = '\0';
	if( fnm != NULL )	*fnm = '\0';
	if( ext != NULL )	*ext = '\0';
	if( *comln == '\0' )	return;

	/* �R�}���h���C���擪�����̎��݂���p�X���� ppp �ɏ����o���B */
	GetExistPath( ppp , comln );

	if( *ppp != '\0' ) {	/* �t�@�C���E�f�B���N�g�������݂���ꍇ */
		/* �擪�������h���C�u���^�[���ǂ������肵�A
		�@ pd = �f�B���N�g�����̐擪�ʒu�ɐݒ肷��B */
		pd = ppp;
		if(
			( *(pd+1)==':' )&&
			( ( ( *pd >='A' )&&( *pd <='Z' ) ) || ( ( *pd >='a' )&&( *pd <='z' ) ) )
		){	/* �擪�Ƀh���C�u���^�[������B */
			pd += 2;	/* pd = �h���C�u���^�[���̌��         */
		}				/*      ( = �f�B���N�g�����̐擪�ʒu ) */
		/* �����܂łŁApd = �f�B���N�g�����̐擪�ʒu */

		attr =  GetFileAttributes(ppp);
		a_dir = ( attr & FILE_ATTRIBUTE_DIRECTORY ) ?  1 : 0;
		if( ! a_dir ){	/* �����������t�@�C���������ꍇ�B */
			pf = sjis_strrchr2(ppp,'\\','\\');	/* �Ŗ����� \ ��T���B */
			if(pf != NULL)	pf++;		/* ����������  pf=\�̎��̕����̈ʒu*/
			else			pf = pd;	/* ������Ȃ���pf=�p�X���̐擪�ʒu */
			/* �����܂ł� pf = �t�@�C�����̐擪�ʒu */
			pe = sjis_strrchr2(pf,'.','.');		/* �Ŗ����� '.' ��T���B */
			if( pe != NULL ){					/* ��������(pe = '.'�̈ʒu)*/
				if( ext != NULL ){	/* �g���q��Ԓl�Ƃ��ď������ށB */
					strncpy(ext,pe,_MAX_EXT -1);
					ext[_MAX_EXT -1] = '\0';
				}
				*pe = '\0';	/* ��؂�ʒu�𕶎���I�[�ɂ���Bpe = �g���q���̐擪�ʒu�B */
			}
			if( fnm != NULL ){	/* �t�@�C������Ԓl�Ƃ��ď������ށB */
				strncpy(fnm,pf,_MAX_FNAME -1);
				fnm[_MAX_FNAME -1] = '\0';
			}
			*pf = '\0';	/* �t�@�C�����̐擪�ʒu�𕶎���I�[�ɂ���B */
		}
		/* �����܂łŕ����� ppp �̓h���C�u���^�[�{�f�B���N�g�����݂̂ɂȂ��Ă��� */
		if( dir != NULL ){
			/* �f�B���N�g�����̍Ō�̕����� \ �ł͂Ȃ��ꍇ�A\ �ɂ���B */

			/* ���Ō�̕����� ch �ɓ���B(�f�B���N�g�������񂪋�̏ꍇ ch='\\' �ƂȂ�) */
			for( ch = '\\' , pf = pd ; *pf != '\0' ; pf++ ){
				ch = *pf;
				if( _IS_SJIS_1(*pf) )	pf++;	/* Shift_JIS ��1�����ڂȂ玟��1�������X�L�b�v */
			}
			/* �����񂪋�łȂ��A���A�Ō�̕����� \ �łȂ������Ȃ�� \ ��ǉ��B */
			if( ( ch != '\\' ) && ( strlen(ppp) < _MAX_PATH -1 ) ){
				*pf++ = '\\';	*pf = '\0';
			}

			/* �f�B���N�g������Ԓl�Ƃ��ď������ށB */
			strncpy(dir,pd,_MAX_DIR -1);
			dir[_MAX_DIR -1] = '\0';
		}
		*pd = '\0';		/* �f�B���N�g�����̐擪�ʒu�𕶎���I�[�ɂ���B */
		if( drv != NULL ){	/* �h���C�u���^�[��Ԓl�Ƃ��ď������ށB */
			strncpy(drv,ppp,_MAX_DRIVE -1);
			drv[_MAX_DRIVE -1] = '\0';
		}
	}
	return;
}
