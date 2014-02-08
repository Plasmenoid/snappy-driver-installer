// Main.cpp

#include "StdAfx.h"

#if defined( _WIN32) && defined( _7ZIP_LARGE_PAGES)
#include "../../../../C/Alloc.h"
#endif

#include "Common/MyInitGuid.h"

#include "Common/CommandLineParser.h"
#include "Common/IntToString.h"
#include "Common/MyException.h"
#include "Common/StdOutStream.h"
#include "Common/StringConvert.h"
#include "Common/StringToInt.h"

#include "Windows/Error.h"
#ifdef _WIN32
#include "Windows/MemoryLock.h"
#endif

#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"
#include "../Common/Extract.h"
#ifdef EXTERNAL_CODECS
#include "../Common/LoadCodecs.h"
#endif
#include "../Common/PropIDUtils.h"

#include "BenchCon.h"
#include "ExtractCallbackConsole.h"
#include "List.h"
#include "OpenCallbackConsole.h"
#include "UpdateCallbackConsole.h"

#include "../../MyVersion.h"

using namespace NWindows;
using namespace NFile;
using namespace NCommandLineParser;

HINSTANCE g_hInstance = 0;
extern CStdOutStream *g_StdStream;

static const char *kCopyrightString = "\n7-Zip"
#ifndef EXTERNAL_CODECS
" (A)"
#endif

#ifdef _WIN64
" [64]"
#endif

" " MY_VERSION_COPYRIGHT_DATE "\n";

static const char *kHelpString =
	"\nUsage: 7z"
#ifdef _NO_CRYPTO
	"r"
#else
#ifndef EXTERNAL_CODECS
	"a"
#endif
#endif
	" <command> [<switches>...] <archive_name> [<file_names>...]\n"
	"   	[<@listfiles...>]\n"
	"\n"
	"<Commands>\n"
	"  a: Add files to archive\n"
	"  b: Benchmark\n"
	"  d: Delete files from archive\n"
	"  e: Extract files from archive (without using directory names)\n"
	"  l: List contents of archive\n"
//    "  l[a|t][f]: List contents of archive\n"
//    "    a - with Additional fields\n"
//    "    t - with all fields\n"
//    "    f - with Full pathnames\n"
	"  t: Test integrity of archive\n"
	"  u: Update files to archive\n"
	"  x: eXtract files with full paths\n"
	"<Switches>\n"
	"  -ai[r[-|0]]{@listfile|!wildcard}: Include archives\n"
	"  -ax[r[-|0]]{@listfile|!wildcard}: eXclude archives\n"
	"  -bd: Disable percentage indicator\n"
	"  -i[r[-|0]]{@listfile|!wildcard}: Include filenames\n"
	"  -m{Parameters}: set compression Method\n"
	"  -o{Directory}: set Output directory\n"
	#ifndef _NO_CRYPTO
	"  -p{Password}: set Password\n"
	#endif
	"  -r[-|0]: Recurse subdirectories\n"
	"  -scs{UTF-8 | WIN | DOS}: set charset for list files\n"
	"  -sfx[{name}]: Create SFX archive\n"
	"  -si[{name}]: read data from stdin\n"
	"  -slt: show technical information for l (List) command\n"
	"  -so: write data to stdout\n"
	"  -ssc[-]: set sensitive case mode\n"
	"  -ssw: compress shared files\n"
	"  -t{Type}: Set type of archive\n"
	"  -u[-][p#][q#][r#][x#][y#][z#][!newArchiveName]: Update options\n"
	"  -v{Size}[b|k|m|g]: Create volumes\n"
	"  -w[{path}]: assign Work directory. Empty path means a temporary directory\n"
	"  -x[r[-|0]]]{@listfile|!wildcard}: eXclude filenames\n"
	"  -y: assume Yes on all queries\n";

// ---------------------------
// exception messages

static const char *kEverythingIsOk = "Everything is Ok";
static const char *kUserErrorMessage = "Incorrect command line";
static const char *kNoFormats = "7-Zip cannot find the code that works with archives.";
static const char *kUnsupportedArcTypeMessage = "Unsupported archive type";

static const wchar_t *kDefaultSfxModule = L"7zCon.sfx";

static void ShowMessageAndThrowException(CStdOutStream &s, LPCSTR message, NExitCode::EEnum code)
{
  s << message << endl;
  throw code;
}

static void PrintHelpAndExit(CStdOutStream &s)
{
  s << kHelpString;
  ShowMessageAndThrowException(s, kUserErrorMessage, NExitCode::kUserError);
}

#ifndef _WIN32
static void GetArguments(int numArgs, const char *args[], UStringVector &parts)
{
  parts.Clear();
  for (int i = 0; i < numArgs; i++)
  {
	UString s = MultiByteToUnicodeString(args[i]);
	parts.Add(s);
  }
}
#endif

static void ShowCopyrightAndHelp(CStdOutStream &s, bool needHelp)
{
  s << kCopyrightString;
  // s << "# CPUs: " << (UInt64)NWindows::NSystem::GetNumberOfProcessors() << "\n";
  if (needHelp)
	s << kHelpString;
}

#ifdef EXTERNAL_CODECS
static void PrintString(CStdOutStream &stdStream, const AString &s, int size)
{
  int len = s.Length();
  stdStream << s;
  for (int i = len; i < size; i++)
	stdStream << ' ';
}
#endif

static void PrintString(CStdOutStream &stdStream, const UString &s, int size)
{
  int len = s.Length();
  stdStream << s;
  for (int i = len; i < size; i++)
	stdStream << ' ';
}

static inline char GetHex(Byte value)
{
  return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10)));
}

int Main2(
	WCHAR *command_line
)
{

//    extract2fs(numArgs,args,"",L"",L"");
  #if defined(_WIN32) && !defined(UNDER_CE)
  SetFileApisToOEM();
  #endif

  UStringVector commandStrings;
  #ifdef _WIN32
  NCommandLineParser::SplitCommandLine(command_line?command_line:GetCommandLineW(), commandStrings);
  printf("^^ %ws\n",GetCommandLineW());
  printf("^^ %ws\n",command_line);
  #else
  GetArguments(numArgs, args, commandStrings);
  #endif
  if (commandStrings.Size() == 1)
  {
	ShowCopyrightAndHelp(g_StdOut, true);
	return 0;
  }
  commandStrings.Delete(0);

  CArchiveCommandLineOptions options;

  CArchiveCommandLineParser parser;

  parser.Parse1(commandStrings, options);

  CStdOutStream &stdStream = options.StdOutMode ? g_StdErr : g_StdOut;
  g_StdStream = &stdStream;


  parser.Parse2(options);

  CCodecs *codecs = new CCodecs;
  CMyComPtr<
	#ifdef EXTERNAL_CODECS
	ICompressCodecsInfo
	#else
	IUnknown
	#endif
	> compressCodecsInfo = codecs;
  HRESULT result = codecs->Load();
  if (result != S_OK)
	throw CSystemException(result);

  bool isExtractGroupCommand = true;

  if (codecs->Formats.Size() == 0 &&
		isExtractGroupCommand)
	throw kNoFormats;

  CIntVector formatIndices;

/*  stdStream <<"options.ArcType							:" <<options.ArcType <<"]\n";
  printf("options.StdInMode 						 :%d\n",options.StdInMode   					 );
  printf("options.StdOutMode						 :%d\n",options.StdOutMode  					 );
  printf("options.Command.GetPathMode() 			 :%d\n",options.Command.GetPathMode()   		 );
  printf("options.Command.IsTestMode()  			 :%d\n",options.Command.IsTestMode()			 );
  printf("options.OverwriteMode 					 :%d\n",options.OverwriteMode   				 );
  stdStream <<"options.OutputDir						  :" <<options.OutputDir <<'\n';
  printf("options.YesToAll  						 :%d\n",options.YesToAll						 );
  printf("options.CalcCrc   						 :%d\n",options.CalcCrc 						 );
//  printf("options.ExtractProperties   			   :%d\n",options.ExtractProperties 			   );
  stdStream <<"options.ArchivePathsSorted   			  :"<<options.ArchivePathsSorted[0]<<"]\n";
  stdStream <<"options.ArchivePathsSorted   			  :"<<options.ArchivePathsSorted[1]<<"]\n";
  stdStream <<"options.ArchivePathsFullSorted   		  :"<<options.ArchivePathsFullSorted[0]<<"]\n";
  stdStream <<"options.ArchivePathsFullSorted   		  :"<<options.ArchivePathsFullSorted[1]<<"]\n";
//  printf("options.WildcardCensor.Pairs.Front().Head  :%d\n",options.WildcardCensor.Pairs.Front().Head);
*/
  if (!codecs->FindFormatForArchiveType(options.ArcType, formatIndices))
	throw kUnsupportedArcTypeMessage;

  else if (isExtractGroupCommand)
  {
	  CExtractCallbackConsole *ecs = new CExtractCallbackConsole;
	  CMyComPtr<IFolderArchiveExtractCallback> extractCallback = ecs;

	  ecs->OutStream = &stdStream;


	  ecs->Init();

	  COpenCallbackConsole openCallback;
	  openCallback.OutStream = &stdStream;


	  CExtractOptions eo;
	  eo.StdInMode = options.StdInMode;
	  eo.StdOutMode = options.StdOutMode;
	  eo.PathMode = options.Command.GetPathMode();
	  eo.TestMode = options.Command.IsTestMode();
	  eo.OverwriteMode = options.OverwriteMode;
	  eo.OutputDir = options.OutputDir;
	  eo.YesToAll = options.YesToAll;
	  eo.CalcCrc = options.CalcCrc;
	  #if !defined(_7ZIP_ST) && !defined(_SFX)
	  //eo.Properties = options.ExtractProperties;
	  #endif
	  UString errorMessage;
	  CDecompressStat stat;
	  HRESULT result = DecompressArchives(
		  codecs,
		  formatIndices,
		  options.ArchivePathsSorted,
		  options.ArchivePathsFullSorted,
		  options.WildcardCensor.Pairs.Front().Head,
		  eo, &openCallback, ecs, errorMessage, stat);
	  if (!errorMessage.IsEmpty())
	  {
		stdStream << endl << "Error: " << errorMessage;
		if (result == S_OK)
		  result = E_FAIL;
	  }

	  stdStream << endl;
	  if (ecs->NumArchives > 1)
		stdStream << "Archives: " << ecs->NumArchives << endl;
	  if (ecs->NumArchiveErrors != 0 || ecs->NumFileErrors != 0)
	  {
		if (ecs->NumArchives > 1)
		{
		  stdStream << endl;
		  if (ecs->NumArchiveErrors != 0)
			stdStream << "Archive Errors: " << ecs->NumArchiveErrors << endl;
		  if (ecs->NumFileErrors != 0)
			stdStream << "Sub items Errors: " << ecs->NumFileErrors << endl;
		}
		if (result != S_OK)
		  throw CSystemException(result);
		return NExitCode::kFatalError;
	  }
	  if (result != S_OK)
		throw CSystemException(result);
	  if (stat.NumFolders != 0)
		stdStream << "Folders: " << stat.NumFolders << endl;
	  if (stat.NumFiles != 1 || stat.NumFolders != 0)
		  stdStream << "Files: " << stat.NumFiles << endl;
	  stdStream
		   << "Size:	   " << stat.UnpackSize << endl
		   << "Compressed: " << stat.PackSize << endl;
  }
  else
	PrintHelpAndExit(stdStream);
  return 0;
}
