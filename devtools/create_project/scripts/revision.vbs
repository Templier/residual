'
' ScummVM - Graphic Adventure Engine
'
' ScummVM is the legal property of its developers, whose names
' are too numerous to list here. Please refer to the COPYRIGHT
' file distributed with this source distribution.
'
' This program is free software; you can redistribute it and/or
' modify it under the terms of the GNU General Public License
' as published by the Free Software Foundation, version 2
' of the License.
'
' This program is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
' GNU General Public License for more details.
'
' You should have received a copy of the GNU General Public License
' along with this program; if not, write to the Free Software
' Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
'
' Based off OpenTTD determineversion.vbs (released under GPL version 2)
'
'/

Option Explicit

' Working copy check priority:
'   True:   TortoiseSVN -> SVN -> Git -> Hg
'   False:  Git -> Hg -> TortoiseSVN -> SVN
Dim prioritySVN: prioritySVN = False

Dim FSO : Set FSO = CreateObject("Scripting.FileSystemObject")
Dim WshShell : Set WshShell = CreateObject("WScript.Shell")

' Folders
Dim rootFolder : rootFolder = ""

' Info variables
Dim tool : tool = ""
Dim branch : branch = "trunk"
Dim revision : revision = ""
Dim modified : modified = False

' Parse our command line arguments
If ParseCommandLine() Then
	' Determine the revision and update the props file with the revision numbers
	DetermineRevision()
End If

'////////////////////////////////////////////////////////////////
'// Revision checking
'////////////////////////////////////////////////////////////////
Sub DetermineRevision()
	Wscript.StdErr.WriteLine "Determining current revision:"

	' Set the current directory to the root folder
	WshShell.CurrentDirectory = rootFolder

	' Try until we find a proper working copy
	If (prioritySVN) Then
		If Not DetermineTortoiseSVNVersion() Then
			If Not DetermineSVNVersion() Then
				If Not DetermineGitVersion() Then
					If Not DetermineHgVersion() Then
						Wscript.StdErr.WriteLine "Could not determine the current revision, skipping..."
						Exit Sub
					End If
				End If
			End If
		End If
	Else
		If Not DetermineGitVersion() Then
			If Not DetermineHgVersion() Then
				If Not DetermineTortoiseSVNVersion() Then
					If Not DetermineSVNVersion() Then
						Wscript.StdErr.WriteLine "Could not determine the current revision, skipping..."
						Exit Sub
					End If
				End If
			End If
		End If
	End If

	Wscript.StdErr.WriteLine "Found revision " & revision & " on branch " & branch & vbCrLf

	' Setup our revision string
	Dim revisionString : revisionString = "r" & revision

	If (modified) Then
		revisionString = revisionString & " M"
	End If

	' If we are not on trunk, add the branch name to the revision string
	If (branch <> "trunk" And branch <> "") Then
		revisionString = revisionString & " (" & branch & ")"
	End If

	' Add the DVCS name at the end
	revisionString = revisionString & " - " & tool

	' Setup an environment variable with the revision string
	Dim Env: Set Env = WshShell.Environment("User")
	Env.item("RESIDUAL_REVISION_STRING") = revisionString
End Sub

Function DetermineTortoiseSVNVersion()
	Err.Clear
	On Error Resume Next
	DetermineTortoiseSVNVersion = False
	Wscript.StdErr.Write "   TortoiseSVN...   "
	tool = "svn"

	' Get the directory where TortoiseSVN (should) reside(s)
	Dim sTortoise

	' First, try with 32-bit architecture
	sTortoise = ReadRegistryKey("HKLM", "SOFTWARE\TortoiseSVN", "Directory", 32)

	If sTortoise = "" Or IsNull(sTortoise) Then
		' No 32-bit version of TortoiseSVN installed, try 64-bit version (doesn't hurt on 32-bit machines, it returns nothing or is ignored)
		sTortoise = ReadRegistryKey("HKLM", "SOFTWARE\TortoiseSVN", "Directory", 64)
	End If

	' Check if Tortoise is present
	If sTortoise = "" Then
		Wscript.StdErr.WriteLine "TortoiseSVN not installed!"
		Exit Function
	End If

	' If TortoiseSVN is installed, try to get the revision number
	Dim SubWCRev : Set SubWCRev = WScript.CreateObject("SubWCRev.object")
	SubWCRev.GetWCInfo rootFolder, 0, 0

	' Check if this is a working copy
	If Not SubWCRev.IsSvnItem Then
		Wscript.StdErr.WriteLine "Not a working copy!"
		Exit Function
	End If

	revision = SubWCRev.Revision

	' Check for modifications
	If SubWCRev.HasModifications Then modified = True

	If revision = "" Then
		Wscript.StdErr.WriteLine "No revision found!"
		Exit Function
	End If

	DetermineTortoiseSVNVersion = True
End Function

Function DetermineSVNVersion()
	Err.Clear
	On Error Resume Next
	DetermineSVNVersion = False
	Wscript.StdErr.Write "   SVN...           "
	tool = "svn"

	' Set the environment to English
	WshShell.Environment("PROCESS")("LANG") = "en"

	' Do we have subversion installed? Check immediately whether we've got a modified WC.
	Dim oExec: Set oExec = WshShell.Exec("svnversion " & rootFolder)
	If Err.Number <> 0 Then
		Wscript.StdErr.WriteLine "SVN not installed!"
		Exit Function
	End If

	' Wait till the application is finished ...
	Do While oExec.Status = 0
		WScript.Sleep 100
	Loop

	Dim line: line = OExec.StdOut.ReadLine()
	If line = "exported" Then
		Wscript.StdErr.WriteLine "Not a working copy!"
		Exit Function
	End If

	If InStr(line, "M") Then
		modified = True
	End If

	' And use svn info to get the correct revision and branch information.
	Set oExec = WshShell.Exec("svn info " & rootFolder)

	If Err.Number <> 0 Then
		Wscript.StdErr.WriteLine "No revision found!"
		Exit Function
	End If

	Do
		line = OExec.StdOut.ReadLine()
		If InStr(line, "Last Changed Rev") Then
			revision = Mid(line, 19)
		End If
	Loop While Not OExec.StdOut.atEndOfStream

	If revision = 0 Then
		Wscript.StdErr.WriteLine "No revision found!"
		Exit Function
	End If

	DetermineSVNVersion = True
End Function

Function DetermineGitVersion()
	Err.Clear
	On Error Resume Next
	DetermineGitVersion = False
	Wscript.StdErr.Write "   Git...           "
	tool = "git"

	' First check if we have both a .git & .svn folders (in case hg-git has been set up to have the git folder at the working copy level)
	If FSO.FolderExists(rootFolder & "/.git") And FSO.FolderExists(rootFolder & "/.hg") Then
		Wscript.StdErr.WriteLine "Mercurial clone with git repository in tree!"
		Exit Function
	End If

	' Set the environment to English
	WshShell.Environment("PROCESS")("LANG") = "en"

	' Detect if we are using msysgit that has a cmd script in the path instead of an exe...
	Dim gitPath : gitPath = "git "
	Dim oExec : Set oExec = WshShell.Exec("git")
	If Err.Number <> 0 Then
		gitPath = "git.cmd "
	End If

	Err.Clear
	Set oExec = WshShell.Exec(gitPath & "rev-parse --verify HEAD")
	If Err.Number <> 0 Then
		Wscript.StdErr.WriteLine "Git not installed!"
		Exit Function
	End If

	' Wait till the application is finished ...
	Do While oExec.Status = 0
		WScript.Sleep 100
	Loop

	If oExec.ExitCode <> 0 Then
		Wscript.StdErr.WriteLine "Error parsing git revision!"
		Exit Function
	End If

	' Get the version hash
	Dim hash: hash = oExec.StdOut.ReadLine()

	' Make sure index is in sync with disk
	Set oExec = WshShell.Exec(gitPath & "update-index --refresh")
	If Err.Number = 0 Then
		' Wait till the application is finished ...
		Do While oExec.Status = 0
			WScript.Sleep 100
		Loop
	End If

	Set oExec = WshShell.Exec(gitPath & "diff-index --exit-code --quiet HEAD " & rootFolder)
	If oExec.ExitCode <> 0 Then
		Wscript.StdErr.WriteLine "Error parsing git revision!"
		Exit Function
	End If

	' Wait till the application is finished ...
	Do While oExec.Status = 0
		WScript.Sleep 100
	Loop

	If oExec.ExitCode = 1 Then
		modified = True
	End If

	' Get branch name
	Set oExec = WshShell.Exec(gitPath & "symbolic-ref HEAD")
	If Err.Number = 0 Then
		Dim line: line = oExec.StdOut.ReadLine()
		line = Mid(line, InStrRev(line, "/") + 1)
		If line <> "master" Then
			branch = line
		End If
	End If

	' Check for svn clones
	Set oExec = WshShell.Exec(gitPath & "log --pretty=format:%s --grep=" & Chr(34) & "^(svn r[0-9]*)" & Chr(34) & " -1 " & rootFolder)
	if Err.Number = 0 Then
		revision = Mid(oExec.StdOut.ReadLine(), 7)
		revision = Mid(revision, 1, InStr(revision, ")") - 1)
		tool = "svn-git"
	End If

	' No revision? Maybe it is a custom git-svn clone
	If revision = "" Then
		Err.Clear
		Set oExec = WshShell.Exec(gitPath & "log --pretty=format:%b --grep=" & Chr(34) & "git-svn-id:.*@[0-9]*" & Chr(34) & " -1 " & rootFolder)
		If Err.Number = 0 Then
			revision = oExec.StdOut.ReadLine()
			revision = Mid(revision, InStr(revision, "@") + 1)
			revision = Mid(revision, 1, InStr(revision, " ") - 1)
			tool = "svn-git"
		End If
	End If

	' Fallback to abbreviated revision number
	If revision = "" Then
		revision = Mid(hash, 1, 8)
	End If

	DetermineGitVersion = True
End Function

Function DetermineHgVersion()
	Err.Clear
	On Error Resume Next
	DetermineHgVersion = False
	Wscript.StdErr.Write "   Mercurial...     "
	tool = "hg"

	Err.Clear
	Dim oExec: Set oExec = WshShell.Exec("hg parents --template ""{rev}:{node|short}""")
	If Err.Number <> 0 Then
		Wscript.StdErr.WriteLine "Mercurial not installed!"
		Exit Function
	End If

	' Wait till the application is finished ...
	Do While oExec.Status = 0
		WScript.Sleep 100
	Loop

	If oExec.ExitCode <> 0 Then
		Wscript.StdErr.WriteLine "Error parsing mercurial revision!"
		Exit Function
	End If

	Dim info : info = Split(OExec.StdOut.ReadLine(), ":")
	Dim version : version = info(0)
	Dim hash : hash = info(1)

	Set oExec = WshShell.Exec("hg status " & rootFolder)
	If Err.Number <> 0 Then
		Wscript.StdErr.WriteLine "Error parsing mercurial revision!"
		Exit Function
	End If

	' Check for modifications
	Do
		line = OExec.StdOut.ReadLine()
		If Len(line) > 0 And Mid(line, 1, 1) <> "?" Then
			modified = True
			Exit Do
		End If
	Loop While Not OExec.StdOut.atEndOfStream

	' Check for branch
	Set oExec = WshShell.Exec("hg branch")
	If Err.Number = 0 Then
		line = OExec.StdOut.ReadLine()
		If line <> "default" Then
			branch = line
		End If
	End If

	' Check for SVN clone
	Set oExec = WshShell.Exec("hg log -f -l 1 --template ""{svnrev}\n"" --cwd " & rootFolder)
	If Err.Number = 0 Then
		revision = Mid(OExec.StdOut.ReadLine(), 7)
		revision = Mid(revision, 1, InStr(revision, ")") - 1)
		tool = "svn-hg"
	End If

	' Fallback to abbreviated revision number
	If revision = "" Then
		revision = version & "(" & hash & ")"
	End If

	DetermineHgVersion = True
End Function

'////////////////////////////////////////////////////////////////
'// Utilities
'////////////////////////////////////////////////////////////////
Function ParseCommandLine()
	ParseCommandLine = True

	If Wscript.Arguments.Count <> 1 Then
		Wscript.StdErr.WriteLine "[Error] Invalid number of arguments (was: " & Wscript.Arguments.Count & ", expected: 1)"

		ParseCommandLine = False
		Exit Function
	End If

	' Get our arguments
	rootFolder = Wscript.Arguments.Item(0)

	' Check that the folders are valid
	If Not FSO.FolderExists(rootFolder) Then
		Wscript.StdErr.WriteLine "[Error] Invalid root folder (" & rootFolder & ")"

		ParseCommandLine = False
		Exit Function
	End If

	' Set absolute path
	rootFolder = FSO.GetAbsolutePathName(rootFolder)
End Function

Function ReadRegistryKey(shive, subkey, valuename, architecture)
	Dim hiveKey, objCtx, objLocator, objServices, objReg, Inparams, Outparams

	' First, get the Registry Provider for the requested architecture
	Set objCtx = CreateObject("WbemScripting.SWbemNamedValueSet")
	objCtx.Add "__ProviderArchitecture", architecture ' Must be 64 of 32
	Set objLocator = CreateObject("Wbemscripting.SWbemLocator")
	Set objServices = objLocator.ConnectServer("","root\default","","",,,,objCtx)
	Set objReg = objServices.Get("StdRegProv")

	' Check the hive and give it the right value
	Select Case shive
		Case "HKCR", "HKEY_CLASSES_ROOT"
			hiveKey = &h80000000
		Case "HKCU", "HKEY_CURRENT_USER"
			hiveKey = &H80000001
		Case "HKLM", "HKEY_LOCAL_MACHINE"
			hiveKey = &h80000002
		Case "HKU", "HKEY_USERS"
			hiveKey = &h80000003
		Case "HKCC", "HKEY_CURRENT_CONFIG"
			hiveKey = &h80000005
		Case "HKDD", "HKEY_DYN_DATA" ' Only valid for Windows 95/98
			hiveKey = &h80000006
		Case Else
			MsgBox "Hive not valid (ReadRegistryKey)"
	End Select

	Set Inparams = objReg.Methods_("GetStringValue").Inparameters
	Inparams.Hdefkey = hiveKey
	Inparams.Ssubkeyname = subkey
	Inparams.Svaluename = valuename
	Set Outparams = objReg.ExecMethod_("GetStringValue", Inparams,,objCtx)

	ReadRegistryKey = Outparams.SValue
End Function