# Microsoft Developer Studio Project File - Name="Encode" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Encode - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Encode.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Encode.mak" CFG="Encode - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Encode - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Encode - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/MP3Stego Encoder", BBAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Encode - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D BS_FORMAT=BINARY /FR /YX /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy    Release\encode.exe    ..\ 
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D BS_FORMAT=BINARY /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR /YX /FD /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Encode - Win32 Release"
# Name "Encode - Win32 Debug"
# Begin Group "Source files"

# PROP Default_Filter "*.c *.cpp"
# Begin Source File

SOURCE=.\bitstream.c
DEP_CPP_BITST=\
	".\bitstream.h"\
	".\error.h"\
	".\ieeefloat.h"\
	".\portableio.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fft.c
DEP_CPP_FFT_C=\
	".\ieeefloat.h"\
	".\l3psy.h"\
	".\layer3.h"\
	".\portableio.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\formatBitstream.c
DEP_CPP_FORMA=\
	".\formatbitstream.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\huffman.c
DEP_CPP_HUFFM=\
	".\bitstream.h"\
	".\huffman.h"\
	".\ieeefloat.h"\
	".\l3bitstream.h"\
	".\layer3.h"\
	".\portableio.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ieeefloat.c
DEP_CPP_IEEEF=\
	".\ieeefloat.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\l3bitstream.c
DEP_CPP_L3BIT=\
	"..\..\StegoLib\stego.h"\
	".\bitstream.h"\
	".\error.h"\
	".\formatbitstream.h"\
	".\huffman.h"\
	".\ieeefloat.h"\
	".\l3bitstream.h"\
	".\l3loop.h"\
	".\l3mdct.h"\
	".\l3psy.h"\
	".\layer3.h"\
	".\portableio.h"\
	".\tables.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\l3loop.c
DEP_CPP_L3LOO=\
	"..\..\StegoLib\stego.h"\
	".\bitstream.h"\
	".\error.h"\
	".\huffman.h"\
	".\ieeefloat.h"\
	".\l3bitstream.h"\
	".\l3loop.h"\
	".\layer3.h"\
	".\portableio.h"\
	".\reservoir.h"\
	".\sqrttab.h"\
	".\tables.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\l3mdct.c
DEP_CPP_L3MDC=\
	".\ieeefloat.h"\
	".\l3mdct.h"\
	".\layer3.h"\
	".\portableio.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\l3psy.c
DEP_CPP_L3PSY=\
	".\error.h"\
	".\fft.h"\
	".\ieeefloat.h"\
	".\l3psy.h"\
	".\layer3.h"\
	".\portableio.h"\
	".\psy_data.h"\
	".\tables.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\l3subband.c
DEP_CPP_L3SUB=\
	".\ieeefloat.h"\
	".\l3subband.h"\
	".\portableio.h"\
	".\tables.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\layer3.c
DEP_CPP_LAYER=\
	"..\..\StegoLib\stego.h"\
	".\bitstream.h"\
	".\error.h"\
	".\ieeefloat.h"\
	".\l3bitstream.h"\
	".\l3loop.h"\
	".\l3mdct.h"\
	".\l3psy.h"\
	".\l3subband.h"\
	".\layer3.h"\
	".\portableio.h"\
	".\types.h"\
	".\wave.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\main.c
DEP_CPP_MAIN_=\
	"..\..\StegoLib\stego.h"\
	".\error.h"\
	".\ieeefloat.h"\
	".\layer3.h"\
	".\portableio.h"\
	".\types.h"\
	".\wave.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\portableio.c
DEP_CPP_PORTA=\
	".\ieeefloat.h"\
	".\portableio.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\reservoir.c
DEP_CPP_RESER=\
	".\bitstream.h"\
	".\error.h"\
	".\huffman.h"\
	".\ieeefloat.h"\
	".\l3bitstream.h"\
	".\l3loop.h"\
	".\layer3.h"\
	".\portableio.h"\
	".\reservoir.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tables.c
DEP_CPP_TABLE=\
	".\ieeefloat.h"\
	".\portableio.h"\
	".\tables.h"\
	".\types.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\wave.c
DEP_CPP_WAVE_=\
	".\error.h"\
	".\ieeefloat.h"\
	".\portableio.h"\
	".\types.h"\
	".\wave.h"\
	

!IF  "$(CFG)" == "Encode - Win32 Release"

!ELSEIF  "$(CFG)" == "Encode - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header files"

# PROP Default_Filter "*.h *.hpp"
# Begin Source File

SOURCE=.\bitstream.h
# End Source File
# Begin Source File

SOURCE=.\error.h
# End Source File
# Begin Source File

SOURCE=.\fft.h
# End Source File
# Begin Source File

SOURCE=.\formatbitstream.h
# End Source File
# Begin Source File

SOURCE=.\huffman.h
# End Source File
# Begin Source File

SOURCE=.\ieeefloat.h
# End Source File
# Begin Source File

SOURCE=.\l3bitstream.h
# End Source File
# Begin Source File

SOURCE=.\l3loop.h
# End Source File
# Begin Source File

SOURCE=.\l3mdct.h
# End Source File
# Begin Source File

SOURCE=.\l3psy.h
# End Source File
# Begin Source File

SOURCE=.\l3subband.h
# End Source File
# Begin Source File

SOURCE=.\layer3.h
# End Source File
# Begin Source File

SOURCE=.\portableio.h
# End Source File
# Begin Source File

SOURCE=.\psy_data.h
# End Source File
# Begin Source File

SOURCE=.\reservoir.h
# End Source File
# Begin Source File

SOURCE=.\sqrttab.h
# End Source File
# Begin Source File

SOURCE=.\tables.h
# End Source File
# Begin Source File

SOURCE=.\types.h
# End Source File
# Begin Source File

SOURCE=.\wave.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Makefile
# End Source File
# End Target
# End Project
